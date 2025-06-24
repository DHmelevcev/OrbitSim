// DHmelevcev 2024

#include "SimSignalHandler.h"
#include "Components/LineBatchComponent.h"
#include "SimCelestialBody.h"
#include "SimBody.h"
#include "SimBaseStation.h"

ASimSignalHandler::ASimSignalHandler()
{
    PrimaryActorTick.bCanEverTick = true;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    LineBatchComponent = CreateDefaultSubobject<ULineBatchComponent>(TEXT("LineBatcher"));
    LineBatchComponent->SetupAttachment(RootComponent);
}

bool ASimSignalHandler::HasCollisions(const FVector& firstPos, const FVector& secondPos) {
    if (!CelestialBodies)
        return false;

    for (size_t k = 0; k < CelestialBodies->Num(); ++k) {
        const FVector& o = (*CelestialBodies)[k]->Position;

        FVector m = FMath::ClosestPointOnSegment(o, firstPos, secondPos);
        
        // closest point inside sphere?
        if ((m - o).SizeSquared() < pow((*CelestialBodies)[k]->Radius * 1010, 2)) {
            if (m != firstPos && m != secondPos) // closest point at start -> station - satellite connection
                return true;

            const FVector& r = m == firstPos ? secondPos : firstPos;

            FVector d = r - m;
            d.Normalize();
            m -= o;
            m.Normalize();

            if (d.Dot(m) < 0.087155742) // arcsin(5deg)
                return true;
        }
    }

    return false;
}

void ASimSignalHandler::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!PhysicBodies || !BaseStations)
        return;

    size_t numB = BaseStations->Num();
    size_t numP = PhysicBodies->Num();
    size_t numSum = static_cast<size_t>(numB + numP);
    Distances.SetNum(pow(numSum, 2));

    size_t signalCnt = 0;

    for (size_t i = 0; i < numP; ++i) {
        const FVector& firstPos = (*PhysicBodies)[i]->Position;
        FVector secondPos = firstPos + ((*PhysicBodies)[i]->TrajectoryLifetime.GetTotalSeconds() / 48) * (*PhysicBodies)[i]->Velocity;
        if (LineBatchComponent->BatchedLines.Num() < signalCnt + 1)
            LineBatchComponent->BatchedLines.Emplace();

        FBatchedLine& line = LineBatchComponent->BatchedLines[signalCnt];
        line.Start = firstPos * 100;
        line.End = secondPos * 100;
        line.Color = FLinearColor(FVector::OneVector * 1);
        ++signalCnt;
    }

    // TODO: parallelable
    //for (size_t i = 0; i < numSum; ++i) {
    //    const FVector& firstPos = i >= numP ? (*BaseStations)[i - numP]->GetActorLocation() / 100 :
    //                                          (*PhysicBodies)[i]->Position;

    //    for (size_t j = i + 1; j < numSum; ++j) {
    //        const FVector& secondPos = j >= numP ? (*BaseStations)[j - numP]->GetActorLocation() / 100 :
    //                                               (*PhysicBodies)[j]->Position;

    //        Distances[i * numSum + j] = HasCollisions(firstPos, secondPos) ? HUGE_VAL :
    //                                                                         (secondPos - firstPos).Length();

    //        //Distances[j * numSum + i] = Distances[i * numSum + j];

    //        if (Distances[i * numSum + j] < HUGE_VAL) {
    //            if (LineBatchComponent->BatchedLines.Num() < signalCnt + 1)
    //                LineBatchComponent->BatchedLines.Emplace();

    //            FBatchedLine& line = LineBatchComponent->BatchedLines[signalCnt];
    //            line.Start = firstPos * 100;
    //            line.End = secondPos * 100;
    //            line.Color = FLinearColor(FVector::OneVector * 1); //

    //            ++signalCnt;
    //        }
    //    }
    //}

    LineBatchComponent->BatchedLines.SetNum(signalCnt);
    LineBatchComponent->MarkRenderStateDirty();
}

void ASimSignalHandler::SetContext(TArray<ASimBody*>& iPhysicBodies,
                                   TArray<ASimCelestialBody*>& iCelestialBodies,
                                   TArray<ASimBaseStation*>& iBaseStations)
{
    PhysicBodies = &iPhysicBodies;
    CelestialBodies = &iCelestialBodies;
    BaseStations = &iBaseStations;
}
