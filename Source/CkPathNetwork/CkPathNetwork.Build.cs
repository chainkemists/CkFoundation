using System.IO;
using UnrealBuildTool;

public class CkPathNetwork : CkModuleRules
{
	public CkPathNetwork(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateIncludePaths.AddRange(new string[] {
		});

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",

			"GameplayTags",
			"DeveloperSettings",
			"NavigationSystem",

			"CkAStar",
			"CkCore",
			"CkEcs",
			"CkEcsExt",
			"CkLabel",
			"CkLog",
			"CkNavigation",
			"CkRecord",
			"CkSettings",
		});
	}
}
