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

inline void ASimGameMode::Integrate
(
	double DeltaTime
)
{
	int32 NumP = PhysicBodies.Num();
	int32 NumC = CelestialBodies.Num();

	FVector r;
	double norm_r;
	double r2;
	double r4;
	double z2;
	FVector dJ2;

	double h = DeltaTime / 2;

	for (const auto& body : CelestialBodies)
	{
		body->P[0] = body->P[1] = body->P[2] = body->P[3] = body->Position;
		body->V[0] = body->V[1] = body->V[2] = body->V[3] = body->Velocity;
		body->A[0] = body->A[1] = body->A[2] = body->A[3] = FVector::Zero();
	}
	for (const auto& body : PhysicBodies)
	{
		body->P[0] = body->P[1] = body->P[2] = body->P[3] = body->Position;
		body->V[0] = body->V[1] = body->V[2] = body->V[3] = body->Velocity;
		body->A[0] = body->A[1] = body->A[2] = body->A[3] = FVector::Zero();
	}

	for (int32 k = 0; k < 4; ++k)
	{
		// calculate acceleration between celestial bodies 
		for (int32 i = 0; i < NumC - 1; ++i)
		{
			ASimCelestialBody& body1 = *CelestialBodies[i];

			for (int32 j = i + 1; j < NumC; ++j)
			{
				ASimCelestialBody& body2 = *CelestialBodies[j];

				if (body1.GM != 0 || body2.GM != 0) {
					r = body2.P[k] - body1.P[k];
					r2 = r.X * r.X + r.Y * r.Y + r.Z * r.Z;
					norm_r = FMath::Sqrt(r2);

					if (body2.GM != 0)
						body1.A[k] += r * (body2.GM /
							(norm_r * r2));

					if (body1.GM != 0)
						body2.A[k] -= r * (body1.GM /
							(norm_r * r2));
				}
			}
		}

		// calculate acceleration between celestial and other bodies
		for (int32 i = NumC; i > 0; --i)
		{
			ASimCelestialBody& c_body = *CelestialBodies[i - 1];

			for (const auto& body2 : PhysicBodies)
			{
				ASimBody& p_body = *body2;

				if (c_body.GM != 0) {
					r = c_body.P[k] - p_body.P[k];
					r2 = r.X * r.X + r.Y * r.Y + r.Z * r.Z;
					norm_r = FMath::Sqrt(r2);

					p_body.A[k] += r * c_body.GM / (norm_r * r2);

					if (c_body.J2 != 0) {
						r4 = r2 * r2;
						z2 = r.Z * r.Z;
						dJ2 = {
							r.X / norm_r * (5 * z2 / r2 - 1),
							r.Y / norm_r * (5 * z2 / r2 - 1),
							r.Z / norm_r * (5 * z2 / r2 - 3),
						};

						p_body.A[k] -= dJ2 * (
							1.5e6 * c_body.J2 * c_body.GM *
							c_body.Radius * c_body.Radius / r4);
					}
				}
			}
		}

		for (int32 i = 1; i < NumC; ++i)
		{
			CelestialBodies[i]->A[k] -= CelestialBodies[0]->A[k];
		}
		for (int32 i = 0; i < NumP; ++i)
		{
			PhysicBodies[i]->A[k] -= CelestialBodies[0]->A[k];
		}

		if (k == 2)
			h = DeltaTime;
		
		if (k == 3)
			break;

		for (int32 i = 1; i < NumC; ++i)
		{
			CelestialBodies[i]->V[k + 1] += h * CelestialBodies[i]->A[k];
			CelestialBodies[i]->P[k + 1] += h * CelestialBodies[i]->V[k] +
										h * h * CelestialBodies[i]->A[k] / 2;
		}
		for (int32 i = 0; i < NumP; ++i)
		{
			PhysicBodies[i]->V[k + 1] += h * PhysicBodies[i]->A[k];
			PhysicBodies[i]->P[k + 1] += h * PhysicBodies[i]->V[k] +
									 h * h * PhysicBodies[i]->A[k] / 2;
		}
	}

	for (int32 i = 1; i < NumC; ++i)
	{
		CelestialBodies[i]->Velocity += (h / 6) * (
			CelestialBodies[i]->A[0] +
		2 *(CelestialBodies[i]->A[1] +
			CelestialBodies[i]->A[2])+
			CelestialBodies[i]->A[3]
		);

		CelestialBodies[i]->Position += (h / 6) * (
			CelestialBodies[i]->V[0] +
		2 *(CelestialBodies[i]->V[1] +
			CelestialBodies[i]->V[2])+
			CelestialBodies[i]->V[3]
		);
	}
	for (int32 i = 0; i < NumP; ++i)
	{
		PhysicBodies[i]->Velocity += (h / 6) * (
			PhysicBodies[i]->A[0] +
		2 *(PhysicBodies[i]->A[1] +
			PhysicBodies[i]->A[2])+
			PhysicBodies[i]->A[3]
		);

		PhysicBodies[i]->Position += (h / 6) * (
			PhysicBodies[i]->V[0] +
		2 *(PhysicBodies[i]->V[1] +
			PhysicBodies[i]->V[2])+
			PhysicBodies[i]->V[3]
		);
	}
}
