#include "CkAngelscriptGenerator/AutoTests/CkAutoTestWrapperGenerator.h"

#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS

// Named, and the helper uniquely named: unity builds concatenate this TU with the sibling scanner
// tests, whose anonymous-namespace `Write_Fixture` is visible at global scope there - a same-named
// helper reached through the using-directive below would be an ambiguous call.
namespace ck_test_autotest_wrapper_source_detection
{
    auto Write_SourceDetectionFixture(
        const FString& InPath,
        const FString& InContents) -> void
    {
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(InPath), /*Tree=*/true);
        FFileHelper::SaveStringToFile(InContents, *InPath,
            FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    }
}

// --------------------------------------------------------------------------------------------------------------------
// The opt-out's SOURCE authority: only a real `A<X>_Actor` declaration counts. A commented-out or
// quoted one must not suppress emission — the raw-text candidate scan sees all three, and only the
// comment/string-aware scanner confirmation separates them.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AutoTestWrapperGenerator_SourceDeclaredWrappers_IgnoresCommentedAndQuotedDecoys,
    "CkAngelscriptGenerator.UnitTests.AutoTestWrapperGenerator.SourceDeclaredWrappers_IgnoresCommentedAndQuotedDecoys",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AutoTestWrapperGenerator_SourceDeclaredWrappers_IgnoresCommentedAndQuotedDecoys::RunTest(const FString&)
{
    using namespace ck_test_autotest_wrapper_source_detection;

    const auto TempRoot = FPaths::ConvertRelativePathToFull(
        FPaths::ProjectIntermediateDir() / TEXT("CkWrapperGenTest_SourceDetection"));
    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);

    // A hand-authored test file: the entity script, then its wrapper at the bottom of the same .as.
    const auto RealFile = TempRoot / TEXT("Real.as");
    Write_SourceDetectionFixture(RealFile, TEXT(
        "class UTestWrapDetect_Real : UCk_AutoTest_Base\n"
        "{\n"
        "    UFUNCTION(BlueprintOverride)\n"
        "    void DoBeginPlay(FCk_Handle InHandle)\n"
        "    {\n"
        "    }\n"
        "}\n"
        "\n"
        "class ATestWrapDetect_Real_Actor : ACk_AutoTestRunner\n"
        "{\n"
        "    default _TimeoutSeconds = 2.0f;\n"
        "}\n"));

    const auto DecoyFile = TempRoot / TEXT("Decoys.as");
    Write_SourceDetectionFixture(DecoyFile, TEXT(
        "// class ATestWrapDetect_LineCommented_Actor : ACk_AutoTestRunner\n"
        "/* class ATestWrapDetect_BlockCommented_Actor : ACk_AutoTestRunner */\n"
        "class UTestWrapDetect_Holder\n"
        "{\n"
        "    UFUNCTION(BlueprintOverride)\n"
        "    void DoBeginPlay(FCk_Handle InHandle)\n"
        "    {\n"
        "        Print(\"class ATestWrapDetect_InString_Actor\");\n"
        "    }\n"
        "}\n"));

    const auto BystanderFile = TempRoot / TEXT("Bystander.as");
    Write_SourceDetectionFixture(BystanderFile, TEXT(
        "class ATestWrapDetect_Bystander : ACk_AutoTestRunner\n"
        "{\n"
        "}\n"));

    const auto Declared = FCkAutoTestWrapperGenerator::Collect_SourceDeclaredWrapperNames(
        {RealFile, DecoyFile, BystanderFile});

    TestEqual(TEXT("exactly one wrapper is declared in source"), Declared.Num(), 1);
    TestTrue(TEXT("the real wrapper, as a BARE name (no leading A)"),
        Declared.Contains(TEXT("TestWrapDetect_Real_Actor")));

    TestFalse(TEXT("line-commented declaration does not count"),
        Declared.Contains(TEXT("TestWrapDetect_LineCommented_Actor")));
    TestFalse(TEXT("block-commented declaration does not count"),
        Declared.Contains(TEXT("TestWrapDetect_BlockCommented_Actor")));
    TestFalse(TEXT("declaration inside a string literal does not count"),
        Declared.Contains(TEXT("TestWrapDetect_InString_Actor")));
    TestFalse(TEXT("a class whose name does not end in _Actor is not a wrapper"),
        Declared.Contains(TEXT("TestWrapDetect_Bystander")));

    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AutoTestWrapperGenerator_SourceDeclaredWrappers_EmptyFileListIsEmptySet,
    "CkAngelscriptGenerator.UnitTests.AutoTestWrapperGenerator.SourceDeclaredWrappers_EmptyFileListIsEmptySet",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AutoTestWrapperGenerator_SourceDeclaredWrappers_EmptyFileListIsEmptySet::RunTest(const FString&)
{
    const auto Declared = FCkAutoTestWrapperGenerator::Collect_SourceDeclaredWrapperNames({});

    TestEqual(TEXT("no files scanned means no wrapper suppresses emission"), Declared.Num(), 0);

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
