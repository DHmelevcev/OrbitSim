// DHmelevcev 2025

#include "SimFileManager.h"
#include "SimGameMode.h"
#include "SimCelestialBody.h"
#include "Developer/DesktopPlatform/Public/IDesktopPlatform.h"
#include "Developer/DesktopPlatform/Public/DesktopPlatformModule.h"
#include "Serialization/JsonSerializer.h"
#include "Components/ListView.h"

void USimFileManager::OpenFileDialog(const FString& DialogTitle,
                                     const FString& DefaultPath,
                                     const FString& FileTypes,
                                     TArray<FString>& OutFileNames) {
	if (!GEngine || !GEngine->GameViewport)
		return;

	void* ParentWindowHandle = GEngine->GameViewport->GetWindow()->GetNativeWindow()->GetOSWindowHandle();
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
		return;

	//Opening the file picker!
	uint32 SelectionFlag = 0; //A value of 0 represents single file selection while a value of 1 represents multiple file selection
	DesktopPlatform->OpenFileDialog(ParentWindowHandle, DialogTitle, DefaultPath, FString(""), FileTypes, SelectionFlag, OutFileNames);
}

void USimFileManager::PrintData(const FString& File) {
	TArray<FString> LoadedText;
	FFileHelper::LoadFileToStringArray(LoadedText, *File);
	for (int32 i = 0; i < LoadedText.Num(); i++) {
		GLog->Log(LoadedText[i]);
	}
}

FString USimFileManager::ReadStringFromFile(const FString& File) {
	FString resultString;
	FFileHelper::LoadFileToString(resultString, *File);
	return resultString;
}

void USimFileManager::ImportSatelitteDataFromFile(const FString& File,
	                                              ASimGameMode* GameMode,
	                                              UListView* ListView) {
	FString jsonString = USimFileManager::ReadStringFromFile(File);
	TSharedPtr<FJsonObject> json = USimFileManager::ReadJsonFromString(jsonString);
	if (!json)
		return;

	const auto& array = json->GetArrayField("Satellites");

	for (const auto& satellite : array) {
		const auto& satelliteData = satellite->AsObject();

		FString name = satelliteData->GetStringField("Name");
		FString bodyName = satelliteData->GetStringField("Body");
		double semiMajorAxis = satelliteData->GetNumberField("SemiMajorAxis");
		double eccentricity = satelliteData->GetNumberField("Eccentricity");
		double inclination = satelliteData->GetNumberField("Inclination");
		double longitudeOfAscendingNode = satelliteData->GetNumberField("LongitudeOfAscendingNode");
		double argumentOfPerigee = satelliteData->GetNumberField("ArgumentOfPerigee");
		double trueAnomaly = satelliteData->GetNumberField("TrueAnomaly");

		ASimCelestialBody** body = GameMode->CelestialBodies.FindByPredicate([&bodyName](const auto& body) {
			return body->BodyName == bodyName;
		});
		if (!body || !*body)
			continue;

		ASimBody* newBody = GameMode->SpawnBody(name, *body,
			                                    semiMajorAxis, eccentricity, inclination, longitudeOfAscendingNode, argumentOfPerigee, trueAnomaly);

		if (newBody && ListView)
			ListView->AddItem(newBody);
	}

	return;
}

TSharedPtr<FJsonObject> USimFileManager::ReadJsonFromString(const FString& JsonString) {
	TSharedPtr<FJsonObject> jsonObject;
	bool result = FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(JsonString), jsonObject);
	return result ? jsonObject : nullptr;
}

void USimFileManager::SaveCoverageData(const TArray<double>& iData) {
	TArray<uint8> ByteArray;
	FMemoryWriter Writer(ByteArray);

	uint16 signature = 'MB';
	uint32 totalSize = 14 + 40 + 1024 + 180 * 360;
	uint32 unused = 0;
	uint32 dataOffset = 14 + 40 + 1024;
	uint32 infoSize = 40;
	uint32 width = 360;
	uint32 height = 180;
	uint16 planes = 1;
	uint16 bitCount = 8;
	uint32 compression = 0;
	uint32 imageSize = 180 * 360;
	Writer << signature << totalSize << unused << dataOffset;
	Writer << infoSize << width << height << planes << bitCount << compression << imageSize << unused << unused << unused << unused;

	for (int16 i = 0; i < 256; ++i) {
		uint32 color = -1; // rgb
		if (i > 10)
			color = 255 | 000 << 8 | 0 << 16; // blue
		else if (i > 9)
			color = 255 | 100 << 8 | 0 << 16;
		else if (i > 8)
			color = 255 | 200 << 8 | 0 << 16;
		else if (i > 7)
			color = 200 | 255 << 8 | 0 << 16;
		else if (i > 6)
			color = 100 | 255 << 8 | 0 << 16;
		else if (i > 5)
			color = 0 | 255 << 8 | 000 << 16; // green
		else if (i > 4)
			color = 0 | 255 << 8 | 100 << 16;
		else if (i > 3)
			color = 0 | 255 << 8 | 200 << 16;
		else if (i > 2)
			color = 0 | 200 << 8 | 255 << 16;
		else if (i > 1)
			color = 0 | 100 << 8 | 255 << 16;
		else
			color = 0 | 000 << 8 | 255 << 16; // red
		Writer << color;
	}

	for (int16 j = 0; j < 180; ++j) {
		for (int16 i = 0; i < 360; ++i) {
			uint8 ByteValue = static_cast<uint8>(iData[j * 360 + i]);
			Writer << ByteValue;
		}
	}

	FFileHelper::SaveArrayToFile(ByteArray, *(FPaths::ProjectSavedDir() + "\\log.bmp"));
}
