// DHmelevcev 2024

#include "SimPlayer.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"

ASimPlayer::ASimPlayer() :
	MinCameraDistance(7300.f),
	MaxCameraDistance(1000000000.f)
{
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

void ASimPlayer::BeginPlay()
{
	Super::BeginPlay();
	
	CameraBoom->TargetArmLength = 100000 * DefaultCameraDistance;
}

void ASimPlayer::ZoomCamera(const float Value)
{
	if (Value == 0) return;

	CameraBoom->TargetArmLength *= 1. + Value / 8;

	CameraBoom->TargetArmLength = FMath::Clamp(
		CameraBoom->TargetArmLength,
		100000 * MinCameraDistance,
		100000 * MaxCameraDistance
	);
}

void ASimPlayer::SetMinCameraDistance
(
	const float Radius
)
{
	MinCameraDistance = Radius;

	// if less than twice MinCameraDistance
	if (CameraBoom->TargetArmLength < MinCameraDistance * 200000)
		CameraBoom->TargetArmLength = MinCameraDistance * 200000;
}
