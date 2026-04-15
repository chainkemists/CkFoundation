using System.IO;
using UnrealBuildTool;

public class CkGoap : CkModuleRules
{
	public CkGoap(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateIncludePaths.AddRange(new string[] {
			// ... add other private include paths required here ...
		});

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayTags",

			"CkCore",
			"CkEcs",
			"CkEcsExt",
			"CkAStar",
			"CkLog",
		});
	}
}
