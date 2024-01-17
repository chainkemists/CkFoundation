using System.IO;
using UnrealBuildTool;

public class CkApp : CkModuleRules
{
    public CkApp(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
            // ... add other private include paths required here ...
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",

            "CkAbility",
            "CkActor",
            "CkActorProxy",
            "CkAnimation",
            "CkAttribute",
            "CkBuildConfig",
            "CkCamera",
            "CkCompositeAlgos",
            "CkConsoleCommands",
            "CkCore",
            "CkEcs",
            "CkEcsBasics",
            "CkEditorStyle",
            "CkEntityBridge",
            "CkGameSession",
            "CkGraphics",
            "CkInput",
            "CkIntent",
            "CkLabel",
            "CkLog",
            "CkMemory",
            "CkNet",
            "CkOverlapBody",
            "CkPerception",
            "CkPhysics",
            "CkProfile",
            "CkProjectile",
            "CkProvider",
            "CkRecord",
            "CkResourceLoader",
            "CkResourceLoaderEditor",
            "CkSettings",
            "CkSignal",
            "CkTimer",
            "CkUI",
            "CkVariables",
        });
    }
}
