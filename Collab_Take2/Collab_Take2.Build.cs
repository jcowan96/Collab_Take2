// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class Collab_Take2 : ModuleRules
{
    private string ModulePath
    {
        get { return ModuleDirectory; }
    }

    private string GMSECApiPath
    {
        get { return "C:/Users/jmcowan/Dev/GMSEC_API/"; }
    }

    public Collab_Take2(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });

		PrivateDependencyModuleNames.AddRange(new string[] { "HeadMountedDisplay" });

        LoadGMSECLib();

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}

    public void LoadGMSECLib()
    {
        //PublicAdditionalLibraries.Add(GMSECApiPath + "bin/gmsecapi.dll");
        //PublicLibraryPaths.Add(GMSECApiPath + "bin");

        PublicAdditionalLibraries.Add(GMSECApiPath + "objects/Release/gmsecapi.lib");
        PublicLibraryPaths.Add(GMSECApiPath + "objects/Release");

        PublicIncludePaths.Add(GMSECApiPath + "framework/include");
        PublicDefinitions.Add(string.Format("WITH_GMSEC_LIB_BINDING=1"));
    }
}
