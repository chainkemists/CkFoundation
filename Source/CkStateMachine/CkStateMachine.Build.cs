using System.IO;
using UnrealBuildTool;

public class CkStateMachine : CkModuleRules
{
    public CkStateMachine(ReadOnlyTargetRules Target) : base(Target)
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

            "CkActorRelay",
            "CkCore",
            "CkDynamic",
            "CkEcs",
            "CkLabel",
            "CkLog",
            "CkProvider",
            "CkRecord",
            "CkSettings",
            "CkTimer",
        });
    }
}
