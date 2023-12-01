using System;
using UnrealBuildTool;

public class CkLog : CkModuleRules
{
    public CkLog(ReadOnlyTargetRules Target) : base(Target)
    {
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);


		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
			}
			);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"GameplayTags",
				"DeveloperSettings",

				"CkThirdParty",
				"CkSettings",
				// ... add private dependencies that you statically link with here ...
			}
			);
    }
}