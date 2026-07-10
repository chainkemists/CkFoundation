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
            "EditorSubsystem",
            "AssetRegistry",
            "MeshDescription",
            "PropertyEditor",
            "Slate",
            "SlateCore",
            "StaticMeshDescription",

            "CkAnimation",
            "CkCore",
            "CkEcs",
            "CkLog",
            "CkVat",
        });
    }
}
