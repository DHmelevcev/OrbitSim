// DHmelevcev 2024

#pragma once

#include "CoreMinimal.h"
#include "SimBody.h"
#include "SimCelestialBody.generated.h"

UCLASS()
class ORBITSIM_API ASimCelestialBody : public ASimBody
{
	GENERATED_BODY()
	
public:	
	ASimCelestialBody();

	// Standard gravitational parameter (m^3 / s^2)
	UPROPERTY(EditAnywhere, Category = "OrbitSim|Body")
	double GM;

	// J2 Perturbation
	UPROPERTY(EditAnywhere, Category = "OrbitSim|Body")
	double J2;

	// in km
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OrbitSim|Body")
	double Radius;

	// in seconds
	UPROPERTY(EditAnywhere, Category = "OrbitSim|Body|Rotation")
	double SiderealRotationPeriod;
};
