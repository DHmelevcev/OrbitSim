// DHmelevcev 2024

#pragma once

class ASimPlayer;
class UEnhancedInputComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SimPlayerController.generated.h"

UCLASS()
class ORBITSIM_API ASimPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "OrbitSim|Input")
	UInputMappingContext* InputMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "OrbitSim|Input|Camera Movement")
	UInputAction* ActionLook;

	UPROPERTY(EditDefaultsOnly, Category = "OrbitSim|Input|Camera Movement")
	UInputAction* ActionZoom;

	UPROPERTY(EditDefaultsOnly, Category = "OrbitSim|Input|Time")
	UInputAction* ActionSpeedUp;

	UPROPERTY(EditDefaultsOnly, Category = "OrbitSim|Input|Time")
	UInputAction* ActionSpeedDown;

private:
	ASimPlayer* SimPlayer;

	int* TimeDilation;

protected:
	virtual void BeginPlay() override;

	virtual void OnPossess(APawn* aPawn) override;

public:
	void Look(const FInputActionValue& Value);

	void Zoom(const FInputActionValue& Value);

	UFUNCTION(BlueprintCallable, Category = "OrbitSim|Time")
	void SpeedUp();

	UFUNCTION(BlueprintCallable, Category = "OrbitSim|Time")
	void SpeedDown();
};
