// DHmelevcev 2024

#pragma once

class USpringArmComponent;
class UCameraComponent;

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "SimPlayer.generated.h"

// in km
constexpr float DefaultCameraDistance = 20000.;

UCLASS()
class ORBITSIM_API ASimPlayer : public APawn
{
	GENERATED_BODY()

public:
	ASimPlayer();

	USpringArmComponent* CameraBoom;

	UCameraComponent* FollowCamera;

	// in km
	UPROPERTY(EditAnywhere, Category = "OrbitSim|Camera|Zoom")
	float MinCameraDistance;

	// in km
	UPROPERTY(EditAnywhere, Category = "OrbitSim|Camera|Zoom")
	float MaxCameraDistance;

protected:
	virtual void BeginPlay() override;

public:
	UFUNCTION(BlueprintCallable, Category = "OrbitSim|Camera")
	void ZoomCamera(const float Value);

	UFUNCTION(BlueprintCallable, Category = "OrbitSim|Camera")
	void SetMinCameraDistance(const float Radius);
};
