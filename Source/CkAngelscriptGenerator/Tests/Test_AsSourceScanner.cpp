// Tests for the AS source scanner (source-derived stub synthesis input layer).
//
// Coverage:
//   * Parse_ClassDeclaration — pure-text fixtures: specifier variants,
//     defaults stripped, comment/string immunity, multi-class files,
//     token-boundary name matching, depth-1-only member capture.
//   * Scan_ClassShape — temp-dir trees: AS->AS inheritance flattening
//     (base-first), the AS->C++ reflection boundary, unresolvable bases,
//     Generated/ + _StubRecovery_ exclusion, inheritance cycles.

#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AsSourceScanner.h"

#include "CkAngelscriptGenerator/CkAngelscriptGenerator_SharedUtils.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Reflection/CkReflection_Utils.h"

#include "CkEcsExt/EntityScript/CkEntityScript_WithActor.h"

#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS

using namespace ck::angelscriptgenerator::self_heal;

namespace
{
    auto Write_Fixture(
        const FString& InPath,
        const FString& InContents) -> void
    {
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(InPath), /*Tree=*/true);
        FFileHelper::SaveStringToFile(InContents, *InPath,
            FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Parse_ClassDeclaration: specifier variants, default stripping, comment and
// string immunity, non-exposed exclusion, method bodies skipped.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AsSourceScanner_Parse_SimpleClass,
    "CkAngelscriptGenerator.UnitTests.AsSourceScanner.Parse_SimpleClass",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AsSourceScanner_Parse_SimpleClass::RunTest(const FString&)
{
    const auto Contents = FString{TEXT(
        "// comment mentioning class UTestScan_Fake { with a brace\n"
        "/* block comment\n"
        "   class UTestScan_Fake2 : UNope {\n"
        "*/\n"
        "class UTestScan_Simple : UCk_GenericEntityScript_UE\n"
        "{\n"
        "    UPROPERTY(ExposeOnSpawn)\n"
        "    FTransform SpawnTransform = FTransform::Identity;\n"
        "\n"
        "    UPROPERTY(ExposeOnSpawn, Category = \"Test\", meta = (DisplayName = \"Weights (map)\"))\n"
        "    TMap<FGameplayTag, float32> Weights;\n"
        "\n"
        "    UPROPERTY()\n"
        "    int32 NotExposed = 7;\n"
        "\n"
        "    UPROPERTY(ExposeOnSpawn)\n"
        "    const UCk_InventoryItem_Definition\n"
        "        Definition;\n"
        "\n"
        "    private FCk_Handle _Internal;\n"
        "\n"
        "    UFUNCTION(BlueprintOverride)\n"
        "    void DoBeginPlay(FCk_Handle InHandle)\n"
        "    {\n"
        "        if (true) { Print(\"brace { in string } and a 'class UFakeInString' too\"); }\n"
        "    }\n"
        "}\n")};

    const auto Result = FCkAsSourceScanner::Parse_ClassDeclaration(
        Contents, TEXT("UTestScan_Simple"), TEXT("D:/Test/Fixture.as"));

    TestTrue(TEXT("Found"), Result.Found);
    TestEqual(TEXT("BaseClassName"), Result.BaseClassName, FString{TEXT("UCk_GenericEntityScript_UE")});
    TestEqual(TEXT("exposed property count"), Result.ExposedProperties.Num(), 3);
    if (Result.ExposedProperties.Num() != 3) { return false; }

    TestEqual(TEXT("[0] type"), Result.ExposedProperties[0].TypeText, FString{TEXT("FTransform")});
    TestEqual(TEXT("[0] name (default stripped)"), Result.ExposedProperties[0].Name, FString{TEXT("SpawnTransform")});

    TestEqual(TEXT("[1] type (template with comma)"), Result.ExposedProperties[1].TypeText, FString{TEXT("TMap<FGameplayTag, float32>")});
    TestEqual(TEXT("[1] name"), Result.ExposedProperties[1].Name, FString{TEXT("Weights")});

    TestEqual(TEXT("[2] type (const + multi-line decl collapsed)"), Result.ExposedProperties[2].TypeText, FString{TEXT("const UCk_InventoryItem_Definition")});
    TestEqual(TEXT("[2] name"), Result.ExposedProperties[2].Name, FString{TEXT("Definition")});

    // The fake class names in comments/strings must not parse.
    TestFalse(TEXT("comment class not found"),
        FCkAsSourceScanner::Parse_ClassDeclaration(Contents, TEXT("UTestScan_Fake"), TEXT("D:/Test/Fixture.as")).Found);
    TestFalse(TEXT("block-comment class not found"),
        FCkAsSourceScanner::Parse_ClassDeclaration(Contents, TEXT("UTestScan_Fake2"), TEXT("D:/Test/Fixture.as")).Found);
    TestFalse(TEXT("in-string class not found"),
        FCkAsSourceScanner::Parse_ClassDeclaration(Contents, TEXT("UFakeInString"), TEXT("D:/Test/Fixture.as")).Found);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Parse_ClassDeclaration: multi-class files — member capture must stop at the
// class body's closing brace; token-boundary name matching must not match a
// prefix of a longer class name.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AsSourceScanner_Parse_MultiClassFile,
    "CkAngelscriptGenerator.UnitTests.AsSourceScanner.Parse_MultiClassFile",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AsSourceScanner_Parse_MultiClassFile::RunTest(const FString&)
{
    const auto Contents = FString{TEXT(
        "class UTestScan_First : UCk_GenericEntityScript_UE\n"
        "{\n"
        "    UPROPERTY(ExposeOnSpawn)\n"
        "    int32 FirstProp;\n"
        "}\n"
        "\n"
        "class UTestScan_FirstExtended : UTestScan_First\n"
        "{\n"
        "    UPROPERTY(ExposeOnSpawn)\n"
        "    bool SecondProp;\n"
        "}\n")};

    const auto First = FCkAsSourceScanner::Parse_ClassDeclaration(
        Contents, TEXT("UTestScan_First"), TEXT("D:/Test/Fixture.as"));
    TestTrue(TEXT("First found"), First.Found);
    TestEqual(TEXT("First props (does NOT leak sibling class members)"), First.ExposedProperties.Num(), 1);
    if (First.ExposedProperties.Num() == 1)
    { TestEqual(TEXT("First prop name"), First.ExposedProperties[0].Name, FString{TEXT("FirstProp")}); }

    const auto Extended = FCkAsSourceScanner::Parse_ClassDeclaration(
        Contents, TEXT("UTestScan_FirstExtended"), TEXT("D:/Test/Fixture.as"));
    TestTrue(TEXT("Extended found (name is a superstring of First)"), Extended.Found);
    TestEqual(TEXT("Extended base"), Extended.BaseClassName, FString{TEXT("UTestScan_First")});
    TestEqual(TEXT("Extended props"), Extended.ExposedProperties.Num(), 1);

    TestFalse(TEXT("prefix of a longer name does not match"),
        FCkAsSourceScanner::Parse_ClassDeclaration(Contents, TEXT("UTestScan_Firs"), TEXT("D:/Test/Fixture.as")).Found);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Scan_ClassShape: AS->AS inheritance chain flattens base-first across files.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AsSourceScanner_ScanShape_AsToAsChain,
    "CkAngelscriptGenerator.UnitTests.AsSourceScanner.ScanShape_AsToAsChain",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AsSourceScanner_ScanShape_AsToAsChain::RunTest(const FString&)
{
    const auto TempRoot = FPaths::ConvertRelativePathToFull(
        FPaths::ProjectIntermediateDir() / TEXT("CkScannerTest_AsChain"));
    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);

    const auto ScriptRoot = TempRoot / TEXT("Script");
    Write_Fixture(ScriptRoot / TEXT("Base.as"), TEXT(
        "class UTestScanChain_Base\n"
        "{\n"
        "    UPROPERTY(ExposeOnSpawn)\n"
        "    FBb_Fragment_DayCycle_ParamsData Params;\n"
        "\n"
        "    UPROPERTY(ExposeOnSpawn)\n"
        "    bool RestoreOnConstruct = false;\n"
        "}\n"));
    Write_Fixture(ScriptRoot / TEXT("Sub/Derived.as"), TEXT(
        "class UTestScanChain_Derived : UTestScanChain_Base\n"
        "{\n"
        "    UPROPERTY(ExposeOnSpawn)\n"
        "    int32 ExtraKnob = 3;\n"
        "}\n"));

    const auto Shape = FCkAsSourceScanner::Scan_ClassShape(TEXT("UTestScanChain_Derived"), {ScriptRoot});

    TestTrue(TEXT("Found"), Shape.Found);
    TestEqual(TEXT("root SourceFilePath is the derived class's file"),
        FPaths::GetCleanFilename(Shape.SourceFilePath), FString{TEXT("Derived.as")});
    TestEqual(TEXT("flattened count"), Shape.FlattenedProperties.Num(), 3);
    if (Shape.FlattenedProperties.Num() != 3) { return false; }

    // Base-first, per-class declaration order — positional Params(...) arg
    // order depends on this.
    TestEqual(TEXT("[0] = base.Params"),             Shape.FlattenedProperties[0].Name, FString{TEXT("Params")});
    TestEqual(TEXT("[1] = base.RestoreOnConstruct"), Shape.FlattenedProperties[1].Name, FString{TEXT("RestoreOnConstruct")});
    TestEqual(TEXT("[2] = derived.ExtraKnob"),       Shape.FlattenedProperties[2].Name, FString{TEXT("ExtraKnob")});

    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Scan_ClassShape: the AS->C++ boundary — a base with no .as declaration
// resolves via reflection, contributing ITS flattened exposed properties
// (UCk_EntityScript_WithActor_UE carries _OwningActor) ahead of the AS
// class's own.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AsSourceScanner_ScanShape_CppBoundary,
    "CkAngelscriptGenerator.UnitTests.AsSourceScanner.ScanShape_CppBoundary",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AsSourceScanner_ScanShape_CppBoundary::RunTest(const FString&)
{
    const auto TempRoot = FPaths::ConvertRelativePathToFull(
        FPaths::ProjectIntermediateDir() / TEXT("CkScannerTest_CppBoundary"));
    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);

    const auto ScriptRoot = TempRoot / TEXT("Script");
    Write_Fixture(ScriptRoot / TEXT("WithActorDerived.as"), TEXT(
        "class UTestScanCpp_Derived : UCk_EntityScript_WithActor_UE\n"
        "{\n"
        "    UPROPERTY(ExposeOnSpawn)\n"
        "    FGameplayTag OwnTag;\n"
        "}\n"));

    const auto Shape = FCkAsSourceScanner::Scan_ClassShape(TEXT("UTestScanCpp_Derived"), {ScriptRoot});
    TestTrue(TEXT("Found"), Shape.Found);
    if (NOT Shape.Found)
    {
        AddError(FString::Printf(TEXT("Scan failed: %s"), *Shape.ErrorMessage));
        return false;
    }

    // Expectation computed from the SAME reflection the real generator uses —
    // the scanner's C++-boundary output must match it name-for-name, in order.
    const auto CppProps = UCk_Utils_Reflection_UE::Get_ExposedPropertiesOfClass(
        UCk_EntityScript_WithActor_UE::StaticClass());

    TestEqual(TEXT("flattened = C++ chain + own"),
        Shape.FlattenedProperties.Num(), CppProps.Num() + 1);
    if (Shape.FlattenedProperties.Num() != CppProps.Num() + 1) { return false; }

    for (auto Index = 0; Index < CppProps.Num(); ++Index)
    {
        TestEqual(FString::Printf(TEXT("C++ prop [%d] name"), Index),
            Shape.FlattenedProperties[Index].Name, CppProps[Index]->GetName());
    }
    TestEqual(TEXT("own prop last"),
        Shape.FlattenedProperties.Last().Name, FString{TEXT("OwnTag")});

    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Scan_ClassShape: failure modes — unresolvable base, class not declared
// anywhere, inheritance cycle, and the Generated/ + _StubRecovery_ exclusions.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AsSourceScanner_ScanShape_FailureModes,
    "CkAngelscriptGenerator.UnitTests.AsSourceScanner.ScanShape_FailureModes",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AsSourceScanner_ScanShape_FailureModes::RunTest(const FString&)
{
    const auto TempRoot = FPaths::ConvertRelativePathToFull(
        FPaths::ProjectIntermediateDir() / TEXT("CkScannerTest_Failures"));
    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);

    const auto ScriptRoot = TempRoot / TEXT("Script");

    Write_Fixture(ScriptRoot / TEXT("Orphan.as"), TEXT(
        "class UTestScanFail_Orphan : UNoSuchBase_NeverExists\n"
        "{\n"
        "    UPROPERTY(ExposeOnSpawn)\n"
        "    int32 Prop;\n"
        "}\n"));

    Write_Fixture(ScriptRoot / TEXT("CycleA.as"), TEXT(
        "class UTestScanFail_CycleA : UTestScanFail_CycleB\n"
        "{\n"
        "}\n"));
    Write_Fixture(ScriptRoot / TEXT("CycleB.as"), TEXT(
        "class UTestScanFail_CycleB : UTestScanFail_CycleA\n"
        "{\n"
        "}\n"));

    Write_Fixture(ScriptRoot / TEXT("Generated/Decoy.as"), TEXT(
        "class UTestScanFail_GeneratedDecoy\n"
        "{\n"
        "}\n"));
    Write_Fixture(ScriptRoot / TEXT("_StubRecovery_Decoy.as"), TEXT(
        "class UTestScanFail_StubDecoy\n"
        "{\n"
        "}\n"));

    // Unresolvable base — fails with the base named in the message.
    {
        const auto Shape = FCkAsSourceScanner::Scan_ClassShape(TEXT("UTestScanFail_Orphan"), {ScriptRoot});
        TestFalse(TEXT("orphan: Found = false"), Shape.Found);
        TestTrue(TEXT("orphan: error names the base"),
            Shape.ErrorMessage.Contains(TEXT("UNoSuchBase_NeverExists")));
    }

    // Class not declared anywhere.
    {
        const auto Shape = FCkAsSourceScanner::Scan_ClassShape(TEXT("UTestScanFail_DoesNotExist"), {ScriptRoot});
        TestFalse(TEXT("missing class: Found = false"), Shape.Found);
        TestFalse(TEXT("missing class: error populated"), Shape.ErrorMessage.IsEmpty());
    }

    // Inheritance cycle terminates with an error instead of looping.
    {
        const auto Shape = FCkAsSourceScanner::Scan_ClassShape(TEXT("UTestScanFail_CycleA"), {ScriptRoot});
        TestFalse(TEXT("cycle: Found = false"), Shape.Found);
        TestTrue(TEXT("cycle: error mentions cycle"),
            Shape.ErrorMessage.Contains(TEXT("cycle")));
    }

    // Generated/ subtree and _StubRecovery_ files are excluded from the scan.
    {
        TestFalse(TEXT("Generated/ decoy not found"),
            FCkAsSourceScanner::Scan_ClassShape(TEXT("UTestScanFail_GeneratedDecoy"), {ScriptRoot}).Found);
        TestFalse(TEXT("_StubRecovery_ decoy not found"),
            FCkAsSourceScanner::Scan_ClassShape(TEXT("UTestScanFail_StubDecoy"), {ScriptRoot}).Found);
    }

    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Scan_ClassShape: the drain-scoped FCk_AsSourceScanCache (QW1) must (a) produce
// results identical to the uncached walk, and (b) actually REUSE its enumeration
// + file-contents on subsequent calls. (b) is proven white-box by poisoning the
// cache: if the cache were ignored, a fresh enumeration/read would still find
// the class — so a NotFound proves the cached state was trusted.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AsSourceScanner_ScanShape_DrainCache,
    "CkAngelscriptGenerator.UnitTests.AsSourceScanner.ScanShape_DrainCache",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AsSourceScanner_ScanShape_DrainCache::RunTest(const FString&)
{
    const auto TempRoot = FPaths::ConvertRelativePathToFull(
        FPaths::ProjectIntermediateDir() / TEXT("CkScannerTest_DrainCache"));
    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);

    const auto ScriptRoot = TempRoot / TEXT("Script");
    Write_Fixture(ScriptRoot / TEXT("Base.as"), TEXT(
        "class UTestScanCache_Base\n"
        "{\n"
        "    UPROPERTY(ExposeOnSpawn)\n"
        "    int32 BaseProp;\n"
        "}\n"));
    Write_Fixture(ScriptRoot / TEXT("Sub/Derived.as"), TEXT(
        "class UTestScanCache_Derived : UTestScanCache_Base\n"
        "{\n"
        "    UPROPERTY(ExposeOnSpawn)\n"
        "    int32 DerivedProp;\n"
        "}\n"));

    // ---- (a) Equivalence: cached scan == uncached scan, and the cache fills ----
    auto       Cache    = FCk_AsSourceScanCache{};
    const auto Cached   = FCkAsSourceScanner::Scan_ClassShape(TEXT("UTestScanCache_Derived"), {ScriptRoot}, &Cache);
    const auto Uncached = FCkAsSourceScanner::Scan_ClassShape(TEXT("UTestScanCache_Derived"), {ScriptRoot}, nullptr);

    TestTrue(TEXT("cached: Found"),   Cached.Found);
    TestTrue(TEXT("uncached: Found"), Uncached.Found);
    TestEqual(TEXT("cached vs uncached flattened count"),
        Cached.FlattenedProperties.Num(), Uncached.FlattenedProperties.Num());
    if (Cached.FlattenedProperties.Num() == Uncached.FlattenedProperties.Num())
    {
        for (auto Index = 0; Index < Cached.FlattenedProperties.Num(); ++Index)
        {
            TestEqual(FString::Printf(TEXT("prop [%d] name parity"), Index),
                Cached.FlattenedProperties[Index].Name, Uncached.FlattenedProperties[Index].Name);
        }
    }
    TestTrue(TEXT("cache: enumeration recorded"), Cache.FilesEnumerated);
    TestTrue(TEXT("cache: file list populated"),  Cache.Files.Num() > 0);
    TestTrue(TEXT("cache: contents populated"),    Cache.ContentsByPath.Num() > 0);

    // ---- (b1) Enumeration reuse: a cache marked enumerated to an EMPTY file ----
    // list must NOT re-enumerate — the class becomes unfindable. A fresh walk
    // would find Derived.as, so NotFound proves the cached list was trusted.
    {
        auto Poisoned = FCk_AsSourceScanCache{};
        Poisoned.FilesEnumerated = true; // Files left empty on purpose
        const auto Shape = FCkAsSourceScanner::Scan_ClassShape(
            TEXT("UTestScanCache_Derived"), {ScriptRoot}, &Poisoned);
        TestFalse(TEXT("enumeration reused (empty cached list => NotFound)"), Shape.Found);
    }

    // ---- (b2) Contents reuse: poison the derived file's cached contents to "" --
    // A re-read from disk would still find the class; NotFound proves the cached
    // (poisoned) contents were used instead of re-reading the file.
    {
        auto Cache2 = FCk_AsSourceScanCache{};
        TestTrue(TEXT("warm-up scan found"),
            FCkAsSourceScanner::Scan_ClassShape(TEXT("UTestScanCache_Derived"), {ScriptRoot}, &Cache2).Found);

        auto PoisonedAny = false;
        for (auto& Pair : Cache2.ContentsByPath)
        {
            if (FPaths::GetCleanFilename(Pair.Key) == TEXT("Derived.as"))
            { Pair.Value.Reset(); PoisonedAny = true; }
        }
        TestTrue(TEXT("derived file was in the contents cache to poison"), PoisonedAny);

        const auto Shape = FCkAsSourceScanner::Scan_ClassShape(
            TEXT("UTestScanCache_Derived"), {ScriptRoot}, &Cache2);
        TestFalse(TEXT("contents reused (poisoned cached contents => NotFound)"), Shape.Found);
    }

    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
