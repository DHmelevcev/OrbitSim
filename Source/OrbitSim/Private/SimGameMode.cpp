// DHmelevcev 2024

#include "SimGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "SimBody.h"
#include "SimCelestialBody.h"
#include "SimTrajectories.h"

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

    Trajectories = static_cast<ASimTrajectories*>(
        UGameplayStatics::GetActorOfClass(
            world,
            ASimTrajectories::StaticClass()
        )
    );

    SetOrigin(CelestialBodies[0]);
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
        Integrate(sim_time_direction * dtSeconds);
        UpdatedTo += sim_time_direction * dt;
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

    int32 destroyed_bodies = 0;
    int32 NumP = PhysicBodies.Num();
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

    if (Trajectories != nullptr)
        Trajectories->Update(PassedTime);
}

bool ASimGameMode::SetBodyOrbit
(
    const FString& Name,
    ASimBody *const NewBody,
    ASimCelestialBody *const MainBody,
    double SemiMajorAxis,
    double Eccentricity,
    double Inclination,
    double LongitudeOfAscendingNode,
    double ArgumentOfPerigee,
    double TrueAnomaly
)
{
    double FocalParameter = 1000. * SemiMajorAxis * (1 - Eccentricity * Eccentricity);
    double SpeedInPeriapsis = sqrt(MainBody->GM / FocalParameter);
    
    FVector Radius = FRotator(
        FMath::Sin(FMath::DegreesToRadians
            (ArgumentOfPerigee)) * Inclination,
        -LongitudeOfAscendingNode - ArgumentOfPerigee,
        0.
    ).Vector();
    Radius *= FocalParameter / (1 + Eccentricity/* * cos(TrueAnomaly)*/);

        
    FVector RadialSpeed(Radius);
    RadialSpeed.Normalize();
    RadialSpeed *=	Eccentricity * 0/*sin(TrueAnomaly)*/ * SpeedInPeriapsis;

    FVector TangentialSpeed = FRotator(
        FMath::Cos(FMath::DegreesToRadians
            (ArgumentOfPerigee)) * Inclination,
        -90. - LongitudeOfAscendingNode - ArgumentOfPerigee,
        0.
    ).Vector();
    TangentialSpeed *= SpeedInPeriapsis * (1 + Eccentricity/* * cos(TrueAnomaly)*/);

    NewBody->Position = MainBody->Position + Radius;
    NewBody->Velocity = MainBody->Velocity + TangentialSpeed + RadialSpeed;

    NewBody->PrevTrajectoryPoint = NewBody->Position * 100;
    NewBody->TrajectoryLifetime = ETimespan::TicksPerSecond *
        TWO_PI * sqrt(pow(1000. * SemiMajorAxis, 3) / MainBody->GM);
    NewBody->TrajectoryCounterPeriod = NewBody->TrajectoryLifetime.GetTotalSeconds() /
                                       dtSeconds / NewBody->TrajectoryLines;

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

    Trajectories->LineBatchComponent->BatchedLines = {};
    Trajectories->LinesToDraw = {};
}

inline void ASimGameMode::UpdateTrajectoryLines()
{
    int32 NumP = PhysicBodies.Num();
    int32 NumC = CelestialBodies.Num();

    auto lambda = [&](ASimBody& body, FLinearColor Color) -> void
    {
        if (body.TrajectoryCounter++ ==
            body.TrajectoryCounterPeriod)
        {
            FVector NewTrajectoryPoint = body.Position * 100;

            if (Trajectories != nullptr &&
                body.TrajectoryLifetime > 0 &&
                UpdatedTo + body.TrajectoryLifetime > WorldTime &&
                UpdatedTo - body.TrajectoryLifetime < WorldTime)
            {
                Trajectories->LinesToDraw.Emplace(FBatchedLine(
                    body.PrevTrajectoryPoint,
                    NewTrajectoryPoint,
                    Color,
                    body.TrajectoryLifetime.GetTotalSeconds(),
                    0,
                    0
                ));
            }

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
