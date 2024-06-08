// DHmelevcev 2024

#include "SimPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "SimPlayer.h"
#include "SimGameMode.h"

void ASimPlayerController::BeginPlay()
{
	Super::BeginPlay();

	ASimGameMode* GameModeBase = (ASimGameMode*)GetWorld()->GetAuthGameMode();
	TimeDilation = &GameModeBase->TimeDilation;

	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void ASimPlayerController::OnPossess
(
	APawn* aPawn
)
{
	Super::OnPossess(aPawn);

	SimPlayer = Cast<ASimPlayer>(aPawn);

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(
			GetLocalPlayer()
		);

	UEnhancedInputComponent* EnhancedInputComponent =
		Cast<UEnhancedInputComponent>(InputComponent);

	InputSubsystem->ClearAllMappings();
	InputSubsystem->AddMappingContext(InputMappingContext, 0);
	EnhancedInputComponent->ClearActionBindings();

	// Looking
	EnhancedInputComponent->BindAction(
		ActionLook,
		ETriggerEvent::Triggered, this,
		&ASimPlayerController::Look
	);

	// Zooming
	EnhancedInputComponent->BindAction(
		ActionZoom,
		ETriggerEvent::Triggered, this,
		&ASimPlayerController::Zoom
	);

	// SpeedUp
	EnhancedInputComponent->BindAction(
		ActionSpeedUp,
		ETriggerEvent::Triggered, this,
		&ASimPlayerController::SpeedUp
	);

	// SpeedDown
	EnhancedInputComponent->BindAction(
		ActionSpeedDown,
		ETriggerEvent::Triggered, this,
		&ASimPlayerController::SpeedDown
	);
}

void ASimPlayerController::Look
(
	const FInputActionValue& Value
)
{
	const FVector2D LookVector = Value.Get<FInputActionValue::Axis2D>();

	SimPlayer->AddControllerYawInput(LookVector.X);
	SimPlayer->AddControllerPitchInput(LookVector.Y);
}

void ASimPlayerController::Zoom
(
	const FInputActionValue& Value
)
{
	SimPlayer->ZoomCamera(Value.Get<FInputActionValue::Axis1D>());
}

void ASimPlayerController::SpeedUp()
{
	if (*TimeDilation >= 10000000)
		return;

	if (*TimeDilation < -1)
		*TimeDilation /= 10;
	else if (*TimeDilation == -1)
		*TimeDilation = 0;
	else if (*TimeDilation == 0)
		*TimeDilation = 1;
	else if (*TimeDilation >= 1)
		*TimeDilation *= 10;
}

void ASimPlayerController::SpeedDown()
{
	if (*TimeDilation <= -10000000)
		return;

	if (*TimeDilation > 1)
		*TimeDilation /= 10;
	else if (*TimeDilation == 1)
		*TimeDilation = 0;
	else if (*TimeDilation == 0)
		*TimeDilation = -1;
	else if (*TimeDilation <= -1)
		*TimeDilation *= 10;
}
