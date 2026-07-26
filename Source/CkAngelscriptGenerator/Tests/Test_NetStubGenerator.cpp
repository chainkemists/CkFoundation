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

    // Precedence: the feature segment nearest the leaf that matches a convention wins, so a
    // `Tests/` dir nested inside a plugin feature dir does not shadow the plugin convention.
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

    // Plugins physically nest under the project dir but must still classify as plugin-authored:
    // misclassifying sends a stub to the wrong repo, where every machine lacking the class prunes it.
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
