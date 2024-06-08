// DHmelevcev 2024

#include "SimStar.h"
#include "Kismet/GameplayStatics.h"
#include "Components/DirectionalLightComponent.h"
#include "SimPlayer.h"
#include "Camera/CameraComponent.h"

ASimStar::ASimStar()
{
	PrimaryActorTick.bCanEverTick = true;

	DirectionalLight =
		CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("DirectionalLight"));

	DirectionalLight->SetIntensity(1.);
	DirectionalLight->SetLightSourceAngle(0);
	DirectionalLight->SetupAttachment(RootComponent);
}

void ASimStar::BeginPlay()
{
	Super::BeginPlay();

	SimPlayer = Cast<ASimPlayer>(
		UGameplayStatics::GetPlayerPawn(GetWorld(), 0)
	);
}

void ASimStar::Tick
(
	float DeltaTime
)
{
	if (DirectionalLight)
		DirectionalLight->SetWorldRotation(
			(SimPlayer->GetActorLocation() - GetActorLocation()).Rotation()
		);
}
