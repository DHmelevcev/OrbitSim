// DHmelevcev 2024

#pragma once

class UDirectionalLightComponent;
class ASimPlayer;

#include "CoreMinimal.h"
#include "SimCelestialBody.h"
#include "SimStar.generated.h"

UCLASS()
class ORBITSIM_API ASimStar : public ASimCelestialBody
{
	GENERATED_BODY()

public:
	ASimStar();

	UPROPERTY(VisibleAnywhere, Category = "OrbitSim")
	UDirectionalLightComponent* DirectionalLight;

private:
	ASimPlayer* SimPlayer;

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
};
