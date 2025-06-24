// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class OrbitSim : ModuleRules
{
	public OrbitSim(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "SlateCore", "Json", "UMG" });

		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
