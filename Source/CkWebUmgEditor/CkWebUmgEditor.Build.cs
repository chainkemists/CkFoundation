using UnrealBuildTool;

public class CkWebUmgEditor : CkModuleRules
{
    public CkWebUmgEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "UnrealEd",
            "Projects",

            "CkCore",
            "CkLog",
            "CkWebUmg",
        });
    }
}
