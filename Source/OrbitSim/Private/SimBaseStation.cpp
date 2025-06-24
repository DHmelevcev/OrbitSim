// DHmelevcev 2024

#include "SimBaseStation.h"
#include "SimCelestialBody.h"

ASimBaseStation::ASimBaseStation()
{
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bCanEverTick = true;
}

void ASimBaseStation::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FRotator rotator = CelestialBody->StaticMesh->GetRelativeRotation();
	rotator.Yaw -= Longitude + 180;
	rotator.Pitch += Latitude;

	FVector newLocation = CelestialBody->Position + rotator.Vector() * CelestialBody->Radius * 1e3;
	SetActorLocation(newLocation * 100);
}
