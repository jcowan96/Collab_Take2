// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MyProject3 : ModuleRules
{
    private string GMSEC_Path = "C:/Users/jmcowan/Dev/GMSEC_API";

    public MyProject3(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay" });

        PrivateDependencyModuleNames.AddRange(new string[] { });

        PublicAdditionalLibraries.Add(GMSEC_Path + "/objects/Release/gmsecapi.lib");
        PublicIncludePaths.Add(GMSEC_Path + "/framework/include");
    }
}
