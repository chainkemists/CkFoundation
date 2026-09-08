using System.IO;
using UnrealBuildTool;

// Yoga is a vendored native library. It deliberately avoids CkModuleRules so its
// exception policy stays isolated from framework modules and its upstream source
// can compile without Ck or engine headers.
public class CkYoga : ModuleRules
{
    public CkYoga(ReadOnlyTargetRules Target) : base(Target)
    {
        CppStandard = CppStandardVersion.Cpp20;
        PCHUsage = PCHUsageMode.NoPCHs;
        bUseUnity = false;
        IWYUSupport = IWYUSupport.None;

        // Undefined Yoga dimensions are NaN. Fast math folds its value != value
        // checks to false and prevents intrinsic measurement callbacks.
        FPSemantics = FPSemanticsMode.Precise;

        // Yoga's upstream CMake configuration enables C++ exceptions. Its fatal
        // assertion path throws only when __cpp_exceptions is available.
        bEnableExceptions = true;
        bUseRTTI = false;

        PublicIncludePaths.Add(
            Path.Combine(ModuleDirectory, "Public", "CkYoga", "yoga-3.2.1"));

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
        });

        // Upstream YG_EXPORT uses _WINDLL to export its C ABI. Keep this private:
        // consumers link the CkYoga import library and must not compile their own
        // headers as an exporting Yoga DLL.
        if (Target.Platform == UnrealTargetPlatform.Win64
            && Target.LinkType != TargetLinkType.Monolithic)
        {
            PrivateDefinitions.Add("_WINDLL=1");
        }
    }
}
