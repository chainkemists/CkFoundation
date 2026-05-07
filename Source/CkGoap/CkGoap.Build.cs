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

		// Private — headers don't expose these. CkRecord especially must NOT be public:
		// CkRecord_Fragment.h declares FFragment_RecordOfEntityExtensions, which forces
		// every transitive consumer to link CkEntityExtension. Keeping CkRecord internal
		// to this module's .cpp avoids that propagation.
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"CkLabel",
			"CkRecord",
		});
	}
}
