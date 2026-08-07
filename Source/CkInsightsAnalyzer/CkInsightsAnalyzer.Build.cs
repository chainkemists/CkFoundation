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
            "Json",

            // Trace data access
            "TraceAnalysis",
            "TraceServices",
            "TraceLog",

            // CK dependencies
            "CkCore",
            "CkLog",
        });
    }
}
