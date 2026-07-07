using UnrealBuildTool;

public class CkUsfEditor : CkModuleRules
{
    public CkUsfEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicIncludePaths.Add(ModuleDirectory);

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "RHI",
            "RenderCore",   // GetShaderSourceFilePath (look validator resolves _UshIncludePath)
            "UnrealEd",
            "EditorSubsystem",
            "MaterialEditor",
            "AssetTools",
            "AssetRegistry",

            "CkCore",
            "CkEcs",
            "CkLog",
            "CkUsf",
        });
    }
}
