using System.IO;
using UnrealBuildTool;

public class CkEcsExt : CkModuleRules
{
    public CkEcsExt(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
            // ... add other private include paths required here ...
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
                        "GameplayTags",

            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "DeveloperSettings",
            "NetCore",
            "PhysicsCore",

            "CkActor",
            "CkCore",
            "CkEcs",
            "CkLabel",
            "CkLog",
            "CkRecord",
            "CkSettings",

            // Public: CkEntityHolder_Fragment.h (a public header) includes CkSnapshot/Context/CkSnapshot_Context.h
            // for TFragment_EntityHolder::SerializeSnapshot, so downstream consumers need the include path too.
            "CkSnapshot",
        });

        if (Target.bBuildEditor)
        {
            PublicDependencyModuleNames.AddRange(new string[]
            {
                "UnrealEd",
            });
        }
    }
}
