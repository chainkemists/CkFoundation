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

			"NavigationSystem",   // for ProjectPointToNavigation in DrawNavProjection processor

			"CkCore",
			"CkEcs",
			"CkEcsExt",
			"CkLabel",
			"CkLog",
			"CkRecord",
			"CkSettings",

			"CkPhysics",
			"CkPmg",            // for FProcessor_CrowdAgent_DrawBody_Setup body capsule + cone
			"CkShapes",
			"CkSpatialQuery",

			"CkNavigation",
		});
	}
}
