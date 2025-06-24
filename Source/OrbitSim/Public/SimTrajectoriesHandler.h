// DHmelevcev 2024

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/LineBatchComponent.h"
#include "SimTrajectoriesHandler.generated.h"

UCLASS()
class ORBITSIM_API ASimTrajectoriesHandler : public AActor
{
	GENERATED_BODY()
	
public:	
	ASimTrajectoriesHandler();

	// trajectory lines currently drawn
	ULineBatchComponent* LineBatchComponent;

	// trajectory lines to draw
	TArray<FBatchedLine> LinesToDraw;

public:
	void Update(float PassedTime);
};
