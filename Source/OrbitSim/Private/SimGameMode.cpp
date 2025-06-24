// DHmelevcev 2024

#include "SimGameMode.h"
#include <condition_variable>
#include <mutex>
#include "Kismet/GameplayStatics.h"
#include "SimBody.h"
#include "SimCelestialBody.h"
#include "SimTrajectoriesHandler.h"
#include "SimSignalHandler.h"
#include "SimBaseStation.h"
#include "SimFileManager.h"

std::mutex m;
std::condition_variable cv;
bool logReady = false;

ASimGameMode::ASimGameMode() :
    UpdatedTo(FDateTime::FromUnixTimestamp(0)),
    WorldTime(UpdatedTo),
    TimeDilation(1)
{
    PrimaryActorTick.bStartWithTickEnabled = true;
    PrimaryActorTick.bCanEverTick = true;
}

void ASimGameMode::BeginPlay()
{
    UWorld* world = ASimGameMode::GetWorld();

    TArray<AActor*> actors;
    UGameplayStatics::GetAllActorsOfClass
    (
        world,
        ASimBody::StaticClass(),
        actors
    );

    // add bodies to simulation
    for (const auto& actor : actors)
    {
        auto c_body = Cast<ASimCelestialBody>(actor);

        if (c_body != nullptr)
            CelestialBodies.Emplace(c_body);
        else
            PhysicBodies.Emplace(Cast<ASimBody>(actor));
    }

    UGameplayStatics::GetAllActorsOfClass
    (
        world,
        ASimBaseStation::StaticClass(),
        actors
    );

    // add bases to signal handler
    for (const auto& actor : actors)
    {
        BaseStations.Emplace(Cast<ASimBaseStation>(actor));
    }

    TrajectoriesHandler = static_cast<ASimTrajectoriesHandler*>(
        UGameplayStatics::GetActorOfClass(
            world, ASimTrajectoriesHandler::StaticClass()));

    SignalHandler = static_cast<ASimSignalHandler*>(
        UGameplayStatics::GetActorOfClass(
            world, ASimSignalHandler::StaticClass()));

    if (SignalHandler != nullptr)
        SignalHandler->SetContext(PhysicBodies, CelestialBodies, BaseStations);

    SetOrigin(CelestialBodies[0]);
}

void LogThread(ASimGameMode* GameMode, FDateTime EndLogTime) {
    FDateTime LastLogged = 0;
    TArray<FVector> LoggingPotitions;
    TArray<double> Values;
    Values.SetNum(180 * 360);
    uint32 counter = 0;

    while (true) {
        std::unique_lock lk(m);
        cv.wait(lk, [] { return logReady; });

        double PassedTime = (GameMode->UpdatedTo - LastLogged).GetTotalSeconds();
        if ((EndLogTime - LastLogged).GetTotalSeconds() <= 0) {
            logReady = false;
            lk.unlock();
            cv.notify_all();
            break;
        }
        if (PassedTime < 29) {
            logReady = false;
            lk.unlock();
            cv.notify_all();
            continue;
        }

        LoggingPotitions.Reset();
        for (int32 i = 0; i < GameMode->PhysicBodies.Num(); ++i)
            LoggingPotitions.Emplace(GameMode->PhysicBodies[i]->Position);

        LastLogged = GameMode->UpdatedTo;

        logReady = false;
        lk.unlock();
        cv.notify_all();

        const ASimCelestialBody& CelestialBody = *(GameMode->CelestialBodies)[0];
        FRotator Rotation = CelestialBody.StaticMesh->GetRelativeRotation();

        for (int16 j = 0; j < 180; ++j) {
            for (int16 i = 0; i < 360; ++i) {
                double Longitude = i - 179.5;
                double Latitude = 89.5 - j;

                FVector o = FRotator(Rotation.Pitch + Latitude,
                                     Rotation.Yaw + 180 - Longitude,
                                     0)
                                    .Vector() * CelestialBody.Radius * 1e3;

                for (const auto PhysicBody : GameMode->PhysicBodies) {
                    FVector r = PhysicBody->Position - o;
                    r.Normalize();
                    if (r.Dot(o.GetUnsafeNormal()) >= 0.087155742) // arcsin(5deg)
                        ++Values[j * 360 + i];
                }
            }
        }

        ++counter;
    }

    for (auto& value : Values) {
        value /= counter;
    }

    USimFileManager::SaveCoverageData(Values);
}

void ASimGameMode::StartLog() {
    if (this->LogEnabled)
        return;

    FTimespan maxLifetime = 0;
    for (size_t i = 0; i < PhysicBodies.Num(); ++i) {
        const FTimespan& lifetime = PhysicBodies[i]->TrajectoryLifetime;
        if (lifetime > maxLifetime)
            maxLifetime = lifetime;
    }

    AsyncTask(ENamedThreads::AnyThread, [this, maxLifetime]() {
        this->LogEnabled = true;
        LogThread(this, this->WorldTime + 2 * maxLifetime);
        this->LogEnabled = false;
        cv.notify_all();
    });
}

void ASimGameMode::Tick
(
    float DeltaTime
)
{
    WorldTime += FTimespan::FromSeconds(DeltaTime * TimeDilation);
    int32 sim_time_direction = FMath::Sign((WorldTime - UpdatedTo).GetTicks());

    while (UpdatedTo + dt < WorldTime || UpdatedTo > WorldTime)
    {
        std::unique_lock lk(m);
        cv.wait(lk, [this] { return !logReady || !LogEnabled; });

        UpdatedTo += sim_time_direction * dt;

        Integrate(sim_time_direction * dtSeconds);

        logReady = true;
        lk.unlock();
        cv.notify_all();

        UpdateTrajectoryLines();
    }

    for (const auto& c_body : CelestialBodies)
    {
        c_body->SetActorLocation(c_body->Position * 100);

        // rotate bodies
        if (c_body->SiderealRotationPeriod) {
            FRotator new_rotation;
            new_rotation.Yaw =
                -360. *
                WorldTime.ToUnixTimestamp() /
                c_body->SiderealRotationPeriod;

            c_body->StaticMesh->SetRelativeRotation(new_rotation);
        }
    }

    int32 NumP = PhysicBodies.Num();
    int32 destroyed_bodies = 0;
    for (int32 i = 0; i < NumP - destroyed_bodies; ++i)
    {
        // check collision
        for (const auto& c_body : CelestialBodies)
        {
            if ((PhysicBodies[i]->Position - c_body->Position)
                .Size() < c_body->Radius * 1000)
            {
                std::swap(
                    PhysicBodies[i],
                    PhysicBodies[NumP - destroyed_bodies - 1]
                );

                PhysicBodies[NumP - destroyed_bodies - 1]->Destroy();

                ++destroyed_bodies;
                break;
            }
        }

        PhysicBodies[i]->SetActorLocation(PhysicBodies[i]->Position * 100);
    }
    PhysicBodies.SetNum(NumP - destroyed_bodies);

    float PassedTime = DeltaTime * abs(TimeDilation);

    if (TrajectoriesHandler != nullptr)
        TrajectoriesHandler->Update(PassedTime);
}

ASimBody* ASimGameMode::SpawnBody(const FString& Name, ASimCelestialBody* const MainBody, double SemiMajorAxis, double Eccentricity, double Inclination, double LongitudeOfAscendingNode, double ArgumentOfPerigee, double TrueAnomaly)
{
    if (!NewBodyClass)
        return nullptr;

    ASimBody* SpawnedActor = GetWorld()->SpawnActor<ASimBody>(NewBodyClass, MainBody->GetActorLocation(), FRotator(0.0f, 0.0f, 0.0f));

    bool result = SetBodyOrbit(Name, SpawnedActor, MainBody, SemiMajorAxis, Eccentricity, Inclination, LongitudeOfAscendingNode, ArgumentOfPerigee, TrueAnomaly);
    if (!result) {
        GetWorld()->DestroyActor(SpawnedActor);
        return nullptr;
    }

    return SpawnedActor;
}

bool ASimGameMode::SetBodyOrbit
(
    const FString& Name,
    ASimBody *const NewBody,
    ASimCelestialBody *const MainBody,
    double SemiMajorAxis, // a
    double Eccentricity, // e
    double Inclination, // i
    double LongitudeOfAscendingNode, // Ω
    double ArgumentOfPerigee, // ω
    double TrueAnomaly // θ
)
{
    Inclination = FMath::DegreesToRadians(Inclination);
    LongitudeOfAscendingNode = FMath::DegreesToRadians(LongitudeOfAscendingNode);
    ArgumentOfPerigee = FMath::DegreesToRadians(ArgumentOfPerigee);
    TrueAnomaly = FMath::DegreesToRadians(TrueAnomaly);

    double sini = sin(Inclination);
    double cosi = cos(Inclination);

    double sinΩ = sin(LongitudeOfAscendingNode);
    double cosΩ = cos(LongitudeOfAscendingNode);

    double sinω = sin(ArgumentOfPerigee);
    double cosω = cos(ArgumentOfPerigee);

    double sinθ = sin(TrueAnomaly);
    double cosθ = cos(TrueAnomaly);

    // Eccentric anomaly
    double cosE = (Eccentricity + cosθ) / (1 + Eccentricity * cosθ);
    double sinE = FMath::Sign(sinθ) * sqrt(1 - cosE * cosE);

    double Distance = 1000. * SemiMajorAxis * (1 - Eccentricity * cosE);

    double temp0 = sqrt(MainBody->GM /* µ */ * 1000. * SemiMajorAxis) / Distance;

    // orbital frame velocity
    double OVx = -temp0 * sinE;
    double OVy = temp0 * sqrt(1 - Eccentricity * Eccentricity) * cosE;

    double temp11 = -cosω * cosΩ + sinω * cosi * sinΩ;
    double temp12 = -sinω * cosΩ - cosω * cosi * sinΩ;
    double temp21 = -cosω * sinΩ - sinω * cosi * cosΩ;
    double temp22 = -cosω * cosi * cosΩ + sinω * sinΩ;
    double temp31 =  sinω * sini;
    double temp32 =  cosω * sini;

    FVector Radius(Distance * (cosθ * temp11 - sinθ * temp12),
                  -Distance * (cosθ * temp21 + sinθ * temp22),
                   Distance * (cosθ * temp31 + sinθ * temp32));

    FVector Velocity(OVx * temp11 - OVy * temp12,
                   -(OVx * temp21 + OVy * temp22),
                     OVx * temp31 + OVy * temp32);

    NewBody->Position = MainBody->Position + Radius;
    NewBody->Velocity = MainBody->Velocity + Velocity;

    NewBody->PrevTrajectoryPoint = NewBody->Position * 100;
    NewBody->TrajectoryLifetime = ETimespan::TicksPerSecond * TWO_PI * sqrt(pow(1000. * SemiMajorAxis, 3) / MainBody->GM);
    NewBody->TrajectoryCounterPeriod = NewBody->TrajectoryLifetime.GetTotalSeconds() / dtSeconds / NewBody->TrajectoryLines;

    if (!Name.IsEmpty())
        NewBody->BodyName = Name;

    if (PhysicBodies.Find(NewBody) == INDEX_NONE)
        PhysicBodies.Emplace(NewBody);

    return true;
}

void ASimGameMode::SetOrigin
(
    ASimCelestialBody* NewOrigin
)
{
    int32 Index = CelestialBodies.Find(NewOrigin);

    if (Index < 0)
        return;
    
    std::swap(CelestialBodies[Index], CelestialBodies[0]);
    ASimCelestialBody& origin = *CelestialBodies[0];

    for (auto body = ++CelestialBodies.CreateIterator(); body; ++body)
    {
        (*body)->Velocity -= origin.Velocity;
        (*body)->Position -= origin.Position;

        (*body)->PrevTrajectoryPoint = (*body)->Position * 100;
    }
    for (const auto& body : PhysicBodies)
    {
        body->Velocity -= origin.Velocity;
        body->Position -= origin.Position;

        body->PrevTrajectoryPoint = body->Position * 100;
    }

    origin.Position = origin.Velocity = FVector::Zero();

    if (TrajectoriesHandler != nullptr) {
        TrajectoriesHandler->LineBatchComponent->BatchedLines = {};
        TrajectoriesHandler->LinesToDraw = {};
    }
}

inline void ASimGameMode::UpdateTrajectoryLines()
{
    if (TrajectoriesHandler == nullptr)
        return;

    int32 NumP = PhysicBodies.Num();
    int32 NumC = CelestialBodies.Num();

    auto lambda = [&](ASimBody& body, FLinearColor Color) -> void
    {
        if (body.TrajectoryCounter++ ==
            body.TrajectoryCounterPeriod)
        {
            FVector NewTrajectoryPoint = body.Position * 100;

            TrajectoriesHandler->LinesToDraw.Emplace(FBatchedLine(
                body.PrevTrajectoryPoint,
                NewTrajectoryPoint,
                Color,
                body.TrajectoryLifetime.GetTotalSeconds(),
                0,
                0
            ));

            body.PrevTrajectoryPoint = NewTrajectoryPoint;
            body.TrajectoryCounter = 0;
        }
    };

    for (int32 i = 1; i < NumC; ++i)
    {
        lambda( *CelestialBodies[i],
              { float(i) / (NumC - 1), 1.f, 0 });
    }
    for (int32 i = 0; i < NumP; ++i)
    {
        lambda( *PhysicBodies[i],
              { float(i) / (NumP - 1), 0, 1.f });
    }
}

inline void ASimGameMode::CalculateAccelerations
(
    int32 Stage
)
const
{
    // radius vector between bodies
    FVector r;
    // distance between bodies
    double norm_r;
    // distance square
    double r2;
    // distance power 4
    double r4;
    // z component of radius vector squared
    double z2;
    // direction of J2 pertuberation acceleration
    FVector dJ2;

    // calculate acceleration between celestial bodies 
    for (int32 i = 0; i < CelestialBodies.Num() - 1; ++i)
    {
        ASimCelestialBody& body1 = *CelestialBodies[i];

        for (int32 j = i + 1; j < CelestialBodies.Num(); ++j)
        {
            ASimCelestialBody& body2 = *CelestialBodies[j];

            if (body1.GM == 0 && body2.GM == 0)
                continue;

            r = body2.P[Stage] - body1.P[Stage];
            r2 = r.X * r.X + r.Y * r.Y + r.Z * r.Z;
            norm_r = FMath::Sqrt(r2);

            if (body2.GM != 0)
                body1.A[Stage] += r * (body2.GM / (norm_r * r2));

            if (body1.GM != 0)
                body2.A[Stage] -= r * (body1.GM / (norm_r * r2));
        }
    }

    // calculate acceleration between celestial and other bodies
    for (const auto& body1 : CelestialBodies)
    {
        ASimCelestialBody& c_body = *body1;

        for (const auto& body2 : PhysicBodies)
        {
            ASimBody& p_body = *body2;

            if (c_body.GM == 0)
                continue;

            r = c_body.P[Stage] - p_body.P[Stage];
            r2 = r.X * r.X + r.Y * r.Y + r.Z * r.Z;
            norm_r = FMath::Sqrt(r2);

            p_body.A[Stage] += r * c_body.GM / (norm_r * r2);

            if (c_body.J2 == 0)
                continue;

            r4 = r2 * r2;
            z2 = r.Z * r.Z;
            dJ2 = {
                r.X / norm_r * (5 * z2 / r2 - 1),
                r.Y / norm_r * (5 * z2 / r2 - 1),
                r.Z / norm_r * (5 * z2 / r2 - 3),
            };

            p_body.A[Stage] -= dJ2 * (
                1.5e6 * c_body.J2 * c_body.GM *
                c_body.Radius * c_body.Radius / r4);
        }
    }
}

inline void ASimGameMode::IntegrationStage
(
    ASimBody& Body,
    double StepLength,
    int32 Stage
)
const
{
    Body.V[Stage + 1] += StepLength * Body.A[Stage];

    Body.P[Stage + 1] += StepLength * Body.V[Stage] +
        StepLength * StepLength * Body.A[Stage] / 2;
}

void ASimGameMode::IntegrationStep
(
    ASimBody& Body,
    double StepLength
)
const
{
    Body.Velocity += (StepLength / 6) * (
        Body.A[0] +
        2 * (Body.A[1] + Body.A[2]) +
        Body.A[3]
        );

    Body.Position += (StepLength / 6) * (
        Body.V[0] +
        2 * (Body.V[1] + Body.V[2]) +
        Body.V[3]
        );
}

inline void ASimGameMode::Integrate
(
    double DeltaTime
)
{
    double h = DeltaTime / 2;

    // set default values
    for (const auto& body : CelestialBodies)
        body->ClearBuffers();

    for (const auto& body : PhysicBodies)
        body->ClearBuffers();

    // 4 stages of integration
    for (int32 k = 0; k < 4; ++k)
    {
        CalculateAccelerations(k);

        for (int32 i = 1; i < CelestialBodies.Num(); ++i)
        {
            CelestialBodies[i]->A[k] -= CelestialBodies[0]->A[k];
        }
        for (const auto& body : PhysicBodies)
        {
            body->A[k] -= CelestialBodies[0]->A[k];
        }

        if (k == 2)
            h = DeltaTime;
        
        if (k == 3)
            break;

        for (int32 i = 1; i < CelestialBodies.Num(); ++i)
            IntegrationStage(*CelestialBodies[i], h, k);

        for (const auto& body : PhysicBodies)
            IntegrationStage(*body, h, k);
    }

    for (int32 i = 1; i < CelestialBodies.Num(); ++i)
        IntegrationStep(*CelestialBodies[i], DeltaTime);

    for (const auto& body : PhysicBodies)
        IntegrationStep(*body, DeltaTime);
}
