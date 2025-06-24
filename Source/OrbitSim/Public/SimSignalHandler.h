// DHmelevcev 2024

#pragma once

class ASimBody;
class ASimCelestialBody;
class ASimBaseStation;

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SimSignalHandler.generated.h"

UCLASS()
class ORBITSIM_API ASimSignalHandler : public AActor
{
	GENERATED_BODY()
	
public:
	ASimSignalHandler();

private:
	// signal lines currently drawn
	ULineBatchComponent* LineBatchComponent;

	TArray<double> Distances;

	TArray<ASimBody*>* PhysicBodies = nullptr;
	TArray<ASimCelestialBody*>* CelestialBodies = nullptr;
	TArray<ASimBaseStation*>* BaseStations = nullptr;

public:
	virtual void Tick(float DeltaTime) override;

	void SetContext(TArray<ASimBody*>& PhysicBodies,
		            TArray<ASimCelestialBody*>& CelestialBodies,
		            TArray<ASimBaseStation*>& BaseStations);

private:
	bool HasCollisions(const FVector& firstPos, const FVector& secondPos);
};
