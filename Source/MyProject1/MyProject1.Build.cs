// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MyProject1 : ModuleRules
{
	public MyProject1(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
            "GameplayTasks",   // Åöí«â¡
            "NavigationSystem",// Åöí«â¡
            "StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
            "PhysicsCore"

        });

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"MyProject1",
			"MyProject1/Variant_Platforming",
			"MyProject1/Variant_Platforming/Animation",
			"MyProject1/Variant_Combat",
			"MyProject1/Variant_Combat/AI",
			"MyProject1/Variant_Combat/Animation",
			"MyProject1/Variant_Combat/Gameplay",
			"MyProject1/Variant_Combat/Interfaces",
			"MyProject1/Variant_Combat/UI",
			"MyProject1/Variant_SideScrolling",
			"MyProject1/Variant_SideScrolling/AI",
			"MyProject1/Variant_SideScrolling/Gameplay",
			"MyProject1/Variant_SideScrolling/Interfaces",
			"MyProject1/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
