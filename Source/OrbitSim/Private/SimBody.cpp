// DHmelevcev 2024

#include "SimBody.h"
#include "SimGameMode.h"

ASimBody::ASimBody() :
	BodyName("Body"),
	TrajectoryLifetime(0),
	TrajectoryLines(500),
	TrajectoryCounterPeriod(0),
	TrajectoryCounter(0)
{
	PrimaryActorTick.bCanEverTick = false;
	SetCanBeDamaged(false);

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetUsingAbsoluteRotation(true);

	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetupAttachment(RootComponent);
}

void ASimBody::BeginPlay()
{
	Super::BeginPlay();

	FVector location = GetActorLocation();

	Position = location / 100;

	// set trajectory counter 
	if (TrajectoryLifetime > 0 && TrajectoryLines > 0)
	{
		PrevTrajectoryPoint = location;

		TrajectoryCounterPeriod = TrajectoryLifetime.GetTotalSeconds() /
			ASimGameMode::dtSeconds / TrajectoryLines;
	}
}
