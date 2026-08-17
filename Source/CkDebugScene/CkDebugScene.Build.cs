using System.IO;
using UnrealBuildTool;

public class CkDebugScene : CkModuleRules
{
    public CkDebugScene(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "MeshDescription",
            "StaticMeshDescription",
            "CkCore",
        });

        RuntimeDependencies.Add(Path.Combine(EngineDirectory, "Content", "EngineDebugMaterials", "WireframeMaterial.uasset"), StagedFileType.UFS);
        RuntimeDependencies.Add(Path.Combine(PluginDirectory, "Content", "DebugScene", "M_CkDebugScene_Opaque.uasset"), StagedFileType.UFS);
        RuntimeDependencies.Add(Path.Combine(PluginDirectory, "Content", "DebugScene", "M_CkDebugScene_Translucent.uasset"), StagedFileType.UFS);
    }
}
