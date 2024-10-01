using System.IO;
using UnrealBuildTool;

public class CkNet : CkModuleRules
{
    public CkNet(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
            // ... add other private include paths required here ...
        });

        {
            // HACK: we are including the private headers of IrisCore here because of the InstancedStruct NetSerializer
            //       this is a temporary solution until we upgrade to Unreal 5.5

			var enginePath = Path.GetFullPath(Target.RelativeEnginePath);
			var srcrtPath = enginePath + "Source/Runtime/";
			PublicIncludePaths.Add(srcrtPath + "Experimental/Iris/Core/Private/");
        }

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "DeveloperSettings",
            "GameplayTags",
            "StructUtils",
            "NetCore",

            "Iris",
            "IrisCore",

            "NetworkTimeSync",

            "CkCore",
            "CkEcs",
            "CkLog",
            "CkSettings",
            "CkSignal",
            "CkLabel"
        });
    }
}
