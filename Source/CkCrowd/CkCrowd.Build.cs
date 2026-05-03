using System.IO;
using UnrealBuildTool;

public class CkCrowd : CkModuleRules
{
	public CkCrowd(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateIncludePaths.AddRange(new string[] {
		});

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",

			"DeveloperSettings",

			"GameplayTags",

			"CkCore",
			"CkEcs",
			"CkEcsExt",
			"CkLabel",
			"CkLog",
			"CkRecord",
			"CkSettings",

			"CkPhysics",
			"CkShapes",
			"CkSpatialQuery",

			"CkNavigation",
		});
	}
}
