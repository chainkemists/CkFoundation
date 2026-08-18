using System.IO;
using UnrealBuildTool;

public class CkEditorTools : CkModuleRules
{
    public CkEditorTools(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "DeveloperSettings",
            "Slate",
            "SlateCore",

            // For UCk_Plugin_UserSettings_UE base class.
            "CkSettings",

            // For CK_ENSURE, ck::Format_UE, and the path-only plugin-dir lookup FCkIconStyle uses.
            "CkCore",
        });

        // The typed icon registry (FCkIconStyle) loads these as loose files in every build type;
        // outside the editor they are only staged if declared here.
        foreach (var IconResource in Directory.EnumerateFiles(
            Path.Combine(PluginDirectory, "Resources", "Icons"),
            "*.svg",
            SearchOption.AllDirectories))
        {
            RuntimeDependencies.Add(IconResource, StagedFileType.NonUFS);
        }
    }
}
