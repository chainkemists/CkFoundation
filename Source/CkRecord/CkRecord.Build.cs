using System.IO;
using UnrealBuildTool;

public class CkRecord : CkModuleRules
{
    public CkRecord(ReadOnlyTargetRules Target) : base(Target)
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
            "CkLabel",
            "CkLog",

            // Public: CkRecord_Fragment.h (a public header) includes CkSnapshot/Context/CkSnapshot_Context.h for
            // TFragment_RecordOfEntities::SerializeSnapshot, so downstream consumers need the include path too.
            "CkSnapshot",
        });
    }
}
