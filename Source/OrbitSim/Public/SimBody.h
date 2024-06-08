// DHmelevcev 2024

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SimBody.generated.h"

UCLASS()
class ORBITSIM_API ASimBody : public AActor
{
	GENERATED_BODY()

public:
	ASimBody();

	UPROPERTY(VisibleAnywhere, Category = "OrbitSim")
	UStaticMeshComponent* StaticMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitSim")
	FString BodyName;

	// in m
	FVector Position;

	FVector P[4];

	// in m / s
	UPROPERTY(EditAnywhere, Category = "OrbitSim|Body")
	FVector Velocity;

	FVector V[4];

	// in m / s^2
	FVector A[4];

	FVector PrevTrajectoryPoint;

	UPROPERTY(EditAnywhere, Category = "OrbitSim|Body|Trajectory")
	FTimespan TrajectoryLifetime;

	// how many lines can be used to draw trajectory 
	UPROPERTY(EditAnywhere, Category = "OrbitSim|Body|Trajectory")
	int TrajectoryLines;

	int TrajectoryCounterPeriod;
	int TrajectoryCounter;

protected:
	virtual void BeginPlay() override;
};
