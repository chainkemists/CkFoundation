using System.IO;
using UnrealBuildTool;

public class CkProfile : CkModuleRules
{
    public CkProfile(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",

            // GGameThreadTime / GRenderThreadTime / GRHIThreadTime — the cycle counters `stat unit`
            // reports, read by Get_ThreadTimings.
            "RenderCore",

            // RHIGetGPUFrameCycles and GUsingNullRHI, for the GPU timing and its availability test.
            "RHI",

            "CkCore",
            "CkLog"
        });
    }
}
