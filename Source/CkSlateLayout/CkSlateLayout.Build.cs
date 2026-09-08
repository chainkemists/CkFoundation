using UnrealBuildTool;

public class CkSlateLayout : ModuleRules
{
    public CkSlateLayout(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.NoPCHs;
        CppStandard = CppStandardVersion.Cpp20;
        FPSemantics = FPSemanticsMode.Precise;
        PrivateDependencyModuleNames.Add("CoreUObject"); // Slate text styles copy GC-aware brush/font references.
        PrivateDependencyModuleNames.Add("XmlParser");

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "InputCore",
            "Slate",
            "SlateCore",
            "CkYoga",
        });
    }
}
