using UnrealBuildTool;

public class CkGridEditor : CkModuleRules
{
    public CkGridEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        // The module-root header (CkGridEditor_Log.h, next to this Build.cs) is included from
        // Public/Private subfolder files; add the root so it resolves from anywhere in the module.
        PublicIncludePaths.Add(ModuleDirectory);

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "UnrealEd",
            "EditorFramework",

            "InputCore",
            "Slate",
            "SlateCore",
            "ToolMenus",
            "LevelEditor",
            "PropertyEditor",
            "GameplayTags",
            "GameplayTagsEditor",

            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkGrid",
            "CkEntitySpawner",
            "CkLog",
        });
    }
}
