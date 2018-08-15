// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class Collab_Take2 : ModuleRules
{
    private string GMSEC_Path = "C:/Users/Jack/Dev/GMSEC_API";

    public Collab_Take2(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });

        PublicAdditionalLibraries.Add(GMSEC_Path + "/objects/Release/gmsecapi.lib");
        PublicIncludePaths.Add(GMSEC_Path + "/framework/include");

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
