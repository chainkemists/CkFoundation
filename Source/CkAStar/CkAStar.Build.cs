using System.IO;
using UnrealBuildTool;

public class CkAStar : CkModuleRules
{
	public CkAStar(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateIncludePaths.AddRange(new string[] {
			// ... add other private include paths required here ...
		});

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",

			"CkCore",
			"CkEcs",
			"CkLog",
		});
	}
}
