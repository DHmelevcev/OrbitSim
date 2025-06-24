// DHmelevcev 2025

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SimFileManager.generated.h"

class ASimGameMode;
class FJsonObject;
class UListView;

UCLASS()
class ORBITSIM_API USimFileManager : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "OrbitSim|FileManager")
	static void OpenFileDialog(const FString& DialogTitle, const FString& DefaultPath, const FString& FileTypes, TArray<FString>& OutFileNames);

	UFUNCTION(BlueprintCallable, Category = "OrbitSim|FileManager")
	static void PrintData(const FString& File);

	UFUNCTION(BlueprintCallable, Category = "OrbitSim|FileManager")
	static FString ReadStringFromFile(const FString& File);

	UFUNCTION(BlueprintCallable, Category = "OrbitSim|FileManager")
	static void ImportSatelitteDataFromFile(const FString& File, ASimGameMode* GameMode, UListView* ListView = nullptr);

public:
	static TSharedPtr<FJsonObject> ReadJsonFromString(const FString& JsonString);

	static void SaveCoverageData(const TArray<double>& iData);
};
