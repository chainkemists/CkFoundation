using System.IO;
using UnrealBuildTool;

public class CkInsightsAnalyzer : CkModuleRules
{
    public CkInsightsAnalyzer(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
            ModuleDirectory,
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "Slate",
            "SlateCore",
            "InputCore",
            "Json",

            // Trace data access
            "TraceAnalysis",
            "TraceServices",
            "TraceLog",

            // CK dependencies
            "CkCore",
            "CkLog",
        });

        if (Target.bBuildEditor)
        {
            PublicDependencyModuleNames.AddRange(new string[]
            {
                "UnrealEd",
                "WorkspaceMenuStructure",
                "ToolMenus",
                "TraceInsights",
            });

            // Access private headers from TraceInsights (custom engine fork)
            string TraceInsightsPrivate = Path.Combine(
                EngineDirectory, "Source", "Developer", "TraceInsights", "Private");
            PrivateIncludePaths.Add(TraceInsightsPrivate);
        }
    }
}
