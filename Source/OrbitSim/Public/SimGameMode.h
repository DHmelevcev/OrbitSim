// DHmelevcev 2025

#pragma once

class ASimBody;
class ASimCelestialBody;
class ASimTrajectoriesHandler;
class ASimSignalHandler;
class ASimBaseStation;

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SimGameMode.generated.h"

// Standard gravitational parameter (m^3 / s^2)
constexpr double GM_Earth = 398600441800000.;
constexpr double GM_Moon = 4904869500000.;
constexpr double GM_Sun = 132712440042000000000.;

// J2 Perturbation
constexpr double J2_Earth = 0.00108262668;
constexpr double J2_Moon = 0.0002027;
constexpr double J2_Sun = 0.00000022;

// Radius (km)
constexpr double R_Earth = 6371.;
constexpr double R_Moon = 1737.4;
constexpr double R_Sun = 696340.;

// Sidereal rotation period (s)
constexpr double SRP_Earth = 86164.0905;
constexpr double SRP_Moon = 2360591.5104;
constexpr double SRP_Sun = 2114208.0;

// Geostationary Earth orbit
constexpr double GEO_SMA = 42164;
constexpr double GEO_E = 0;
constexpr double GEO_I = 0;

// Sun-synchronous orbit
constexpr double SSO_SMA = 7272;
constexpr double SSO_E = 0;
constexpr double SSO_I = 99;

// Molniya orbit
constexpr double MOLNIYA_SMA = 26600;
constexpr double MOLNIYA_E = 0.74;
constexpr double MOLNIYA_I = 63.4;
constexpr double MOLNIYA_AP = 270;

// Tundra orbit
constexpr double TUNDRA_SMA = 42164;
constexpr double TUNDRA_E = 0.25;
constexpr double TUNDRA_I = 63.4;
constexpr double TUNDRA_AP = 270;

UCLASS()
class ORBITSIM_API ASimGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	ASimGameMode();

	FDateTime UpdatedTo;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OrbitSim|Time")
	FDateTime WorldTime;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OrbitSim|Time")
	int32 TimeDilation;

	inline static const FTimespan dt = ETimespan::TicksPerSecond * 5;
	inline static const double dtSeconds = dt.GetTotalSeconds();

public:
	bool LogEnabled = false;

public:
	TArray<ASimBody*> PhysicBodies; // all bodies except celestial bodies
	TArray<ASimCelestialBody*> CelestialBodies;

	TArray<ASimBaseStation*> BaseStations;

private:
	ASimTrajectoriesHandler* TrajectoriesHandler = nullptr;
	ASimSignalHandler* SignalHandler = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AActor> NewBodyClass;

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "OrbitSim")
	ASimBody* SpawnBody
	(
		const FString& Name,
		ASimCelestialBody* const MainBody,
		double SemiMajorAxis,
		double Eccentricity,
		double Inclination,
		double LongitudeOfAscendingNode,
		double ArgumentOfPerigee,
		double TrueAnomaly
	);

	UFUNCTION(BlueprintCallable, Category = "OrbitSim|Trajectory")
	void SetOrigin(ASimCelestialBody* NewOrigin);

	UFUNCTION(BlueprintCallable, Category = "OrbitSim|Log")
	void StartLog();

private:
	bool SetBodyOrbit
	(
		const FString& Name,
		ASimBody* const NewBody,
		ASimCelestialBody* const MainBody,
		double SemiMajorAxis,
		double Eccentricity,
		double Inclination,
		double LongitudeOfAscendingNode,
		double ArgumentOfPerigee,
		double TrueAnomaly
	);

	void UpdateTrajectoryLines();

	void CalculateAccelerations(int32 Stage) const;

	void IntegrationStage
	(
		ASimBody& Body,
		double StepLength,
		int32 Stage
	) const;

	void IntegrationStep
	(
		ASimBody& Body,
		double StepLength
	) const;

	void Integrate(double DeltaTime);
};
