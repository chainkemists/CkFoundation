using System.IO;
using UnrealBuildTool;

public class CkNavigation : CkModuleRules
{
	public CkNavigation(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateIncludePaths.AddRange(new string[] {
		});

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",

			"NavigationSystem",
			"AIModule",
			"GameplayTags",

			"CkCore",
			"CkEcs",
			"CkEcsExt",
			"CkLabel",
			"CkLog",
			"CkRecord",
			"CkSettings",
		});
	}
}
