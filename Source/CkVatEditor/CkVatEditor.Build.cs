using System.IO;
using UnrealBuildTool;

public class CkVatEditor : CkModuleRules
{
    public CkVatEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "UnrealEd",

            "CkCore",
            "CkEcs",
            "CkLog",
            "CkVat",
        });
    }
}
