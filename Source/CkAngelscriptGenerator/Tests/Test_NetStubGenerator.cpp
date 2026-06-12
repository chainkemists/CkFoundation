// Tests for the net-stub generator's pure path helpers.
//
// Coverage:
//   * Derive_FeatureFromSourcePath — plugin `Script/Ck<Feature>/` convention (incl. deep
//     nesting), project `Script/Tests/<Feature>/` convention (incl. deep nesting), the `AS`
//     fallback, and the filename-is-never-a-feature guard.
//   * Get_IsProjectAuthoredPath — the project-vs-plugin output-root split. The load-bearing
//     case is "plugins live under the project dir too": a plugin-authored test must NOT
//     classify as project-authored just because `Plugins/` physically nests inside
//     `<ProjectDir>/`. A misclassification here re-creates the cross-repo committed-stub
//     churn the split exists to prevent (a stub committed to the CkTests submodule whose
//     class lives in the project repo gets pruned on every machine lacking that class).

#include "CkAngelscriptGenerator/AutoTests/CkAutoTestNetStubGenerator.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------
// Derive_FeatureFromSourcePath
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_NetStubGenerator_Derive_PluginConvention,
    "CkAngelscriptGenerator.UnitTests.NetStubGenerator.Derive_PluginConvention",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_NetStubGenerator_Derive_PluginConvention::RunTest(const FString&)
{
    TestEqual(TEXT("Script/Ck<Feature> buckets to <Feature>"),
        FCkAutoTestNetStubGenerator::Derive_FeatureFromSourcePath(
            TEXT("D:/Fake/Plugins/CkTests/Script/CkAttribute/CkAutoTest_Net_Float_InitialValueReplicates.as")),
        FString{TEXT("Attribute")});

    TestEqual(TEXT("deep nesting under the feature dir still buckets to <Feature>"),
        FCkAutoTestNetStubGenerator::Derive_FeatureFromSourcePath(
            TEXT("D:/Fake/Plugins/CkTests/Script/CkAttribute/Sub/CkAutoTest_Net_Foo.as")),
        FString{TEXT("Attribute")});

    // Backslash form — generated paths on Windows mix separators freely.
    TestEqual(TEXT("backslash separators normalize before matching"),
        FCkAutoTestNetStubGenerator::Derive_FeatureFromSourcePath(
            TEXT("D:\\Fake\\Plugins\\CkTests\\Script\\CkGrid\\CkAutoTest_Net_Grid_OccupancyReplicates.as")),
        FString{TEXT("Grid")});

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_NetStubGenerator_Derive_ProjectConvention,
    "CkAngelscriptGenerator.UnitTests.NetStubGenerator.Derive_ProjectConvention",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_NetStubGenerator_Derive_ProjectConvention::RunTest(const FString&)
{
    TestEqual(TEXT("Script/Tests/<Feature> buckets to <Feature>"),
        FCkAutoTestNetStubGenerator::Derive_FeatureFromSourcePath(
            TEXT("D:/Fake/Proj/Script/Tests/ChangeablePoster/CkAutoTest_Net_ChangeablePoster_TextureReplicates.as")),
        FString{TEXT("ChangeablePoster")});

    TestEqual(TEXT("deep nesting under the feature dir still buckets to <Feature>"),
        FCkAutoTestNetStubGenerator::Derive_FeatureFromSourcePath(
            TEXT("D:/Fake/Proj/Script/Tests/ChangeablePoster/Sub/CkAutoTest_Net_Foo.as")),
        FString{TEXT("ChangeablePoster")});

    // A `Tests/` dir nested inside a plugin feature dir must not shadow the plugin convention:
    // the feature segment nearest the leaf that matches a convention wins, and `Script/CkFoo`
    // is the first (and only) pair that matches here.
    TestEqual(TEXT("plugin convention claims Script/CkFoo/Tests/ layouts"),
        FCkAutoTestNetStubGenerator::Derive_FeatureFromSourcePath(
            TEXT("D:/Fake/Plugins/CkTests/Script/CkFoo/Tests/CkAutoTest_Net_Bar.as")),
        FString{TEXT("Foo")});

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_NetStubGenerator_Derive_Fallback,
    "CkAngelscriptGenerator.UnitTests.NetStubGenerator.Derive_Fallback",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_NetStubGenerator_Derive_Fallback::RunTest(const FString&)
{
    TestEqual(TEXT("unconventional location falls back to AS"),
        FCkAutoTestNetStubGenerator::Derive_FeatureFromSourcePath(
            TEXT("D:/Fake/Plugins/CkTests/Script/Common/CkAutoTest_Net_Foo.as")),
        FString{TEXT("AS")});

    TestEqual(TEXT("empty path falls back to AS"),
        FCkAutoTestNetStubGenerator::Derive_FeatureFromSourcePath(FString{}),
        FString{TEXT("AS")});

    // The file's own name must never be mistaken for a feature directory — a file directly
    // under `Script/Tests/` has no feature dir, and `Tests/<filename>` must not match.
    TestEqual(TEXT("file directly under Script/Tests/ falls back to AS"),
        FCkAutoTestNetStubGenerator::Derive_FeatureFromSourcePath(
            TEXT("D:/Fake/Proj/Script/Tests/CkAutoTest_Net_Foo.as")),
        FString{TEXT("AS")});

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Get_IsProjectAuthoredPath
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_NetStubGenerator_ProjectAuthored_Classification,
    "CkAngelscriptGenerator.UnitTests.NetStubGenerator.ProjectAuthored_Classification",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_NetStubGenerator_ProjectAuthored_Classification::RunTest(const FString&)
{
    const auto ProjectDir = FString{TEXT("D:/Fake/Proj/")};

    TestTrue(TEXT("source under <ProjectDir>/Script/ is project-authored"),
        FCkAutoTestNetStubGenerator::Get_IsProjectAuthoredPath(
            TEXT("D:/Fake/Proj/Script/Tests/ChangeablePoster/CkAutoTest_Net_Foo.as"), ProjectDir));

    TestTrue(TEXT("classification is case-insensitive"),
        FCkAutoTestNetStubGenerator::Get_IsProjectAuthoredPath(
            TEXT("d:/fake/proj/script/tests/ChangeablePoster/CkAutoTest_Net_Foo.as"), ProjectDir));

    TestTrue(TEXT("backslash separators normalize before matching"),
        FCkAutoTestNetStubGenerator::Get_IsProjectAuthoredPath(
            TEXT("D:\\Fake\\Proj\\Script\\Tests\\X\\CkAutoTest_Net_Foo.as"), ProjectDir));

    // Plugins physically nest under the project dir — they must still classify as
    // plugin-authored. This is the case that guards against re-creating the cross-repo churn.
    TestFalse(TEXT("plugin source under <ProjectDir>/Plugins/ is NOT project-authored"),
        FCkAutoTestNetStubGenerator::Get_IsProjectAuthoredPath(
            TEXT("D:/Fake/Proj/Plugins/CkTests/Script/CkAttribute/CkAutoTest_Net_Foo.as"), ProjectDir));

    TestFalse(TEXT("sibling dir sharing the Script prefix is NOT project-authored"),
        FCkAutoTestNetStubGenerator::Get_IsProjectAuthoredPath(
            TEXT("D:/Fake/Proj/ScriptExtra/CkAutoTest_Net_Foo.as"), ProjectDir));

    TestFalse(TEXT("source outside the project dir is NOT project-authored"),
        FCkAutoTestNetStubGenerator::Get_IsProjectAuthoredPath(
            TEXT("D:/Other/Script/Tests/X/CkAutoTest_Net_Foo.as"), ProjectDir));

    TestFalse(TEXT("empty source path is NOT project-authored"),
        FCkAutoTestNetStubGenerator::Get_IsProjectAuthoredPath(FString{}, ProjectDir));

    TestFalse(TEXT("empty project dir is NOT project-authored"),
        FCkAutoTestNetStubGenerator::Get_IsProjectAuthoredPath(
            TEXT("D:/Fake/Proj/Script/Tests/X/CkAutoTest_Net_Foo.as"), FString{}));

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
