// DHmelevcev 2024

#include "SimTrajectoriesHandler.h"
#include "Components/LineBatchComponent.h"

ASimTrajectoriesHandler::ASimTrajectoriesHandler()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	LineBatchComponent = CreateDefaultSubobject<ULineBatchComponent>(TEXT("LineBatcher"));
	LineBatchComponent->SetupAttachment(RootComponent);
}

void ASimTrajectoriesHandler::Update
(
	float PassedTime
)
{
	for (auto& line : LineBatchComponent->BatchedLines)
	{
		float RemainingLifeTime = line.RemainingLifeTime - PassedTime;
		if (RemainingLifeTime > 0)
			line.RemainingLifeTime = RemainingLifeTime;
		else
			line.RemainingLifeTime = 0.0001;
	};

	if (LinesToDraw.Num()) {
		LineBatchComponent->DrawLines(LinesToDraw);
		LinesToDraw = {};
	}
}
