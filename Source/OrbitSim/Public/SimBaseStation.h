// DHmelevcev 2024

#pragma once

class ASimCelestialBody;

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SimBaseStation.generated.h"

UCLASS()
class ORBITSIM_API ASimBaseStation : public AActor
{
	GENERATED_BODY()
	
public:
	ASimBaseStation();

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OrbitSim|BaseStation", meta = (ClampMin = "-180.0", ClampMax = "180.0", UIMin = "-180.0", UIMax = "180.0"))
	double Longitude = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OrbitSim|BaseStation", meta = (ClampMin = "-90.0", ClampMax = "90.0", UIMin = "-90.0", UIMax = "90.0"))
	double Latitude = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OrbitSim|BaseStation")
	ASimCelestialBody* CelestialBody = nullptr;

public:
	virtual void Tick(float DeltaTime) override;
};
