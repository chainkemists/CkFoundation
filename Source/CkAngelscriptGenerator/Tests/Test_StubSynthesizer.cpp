// Tests for the AS stub synthesizer (Rev 10 dispatcher recovery strategy #1).
//
// Coverage spans:
//   * Pure-string builders (Build_*, Derive_*, Get_*) — input/output shape.
//   * Candidate-file discovery (Find_TargetFile_ByContent, Anchor_ByCallerAsPath).
//   * Sibling-stub IO (Inject_EntityScriptParamsStub end-to-end via
//     temp-file fixtures: writes the sibling, preserves canonical, accumulates
//     distinct accessors, dedups same-accessor re-injects).
//
// Individual test names are self-documenting; the canonical list is whatever
// `IMPLEMENT_SIMPLE_AUTOMATION_TEST` instances live in this file.

#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_StubSynthesizer.h"
#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AsErrorParser.h"

#include "CkCore/Macros/CkMacros.h"

#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS

using namespace ck::angelscriptgenerator::self_heal;

namespace
{
    // Builds a probe-shaped NoMatchingSignatures error for tests that need an
    // FCk_AsParsedError input. Mirrors what Parser produces for the probe-1
    // capture except where overrides are supplied.
    auto Make_ParsedError(
        const TCHAR* InTargetNamespace,
        const TCHAR* InFunctionName,
        const TCHAR* InArgsList,
        const TCHAR* InFilePath = TEXT("D:/Test/Caller.as"),
        int32        InLine     = 491,
        int32        InColumn   = 13) -> FCk_AsParsedError
    {
        auto E = FCk_AsParsedError{};
        E.Kind             = ECk_AsParsedError_Kind::NoMatchingSignatures;
        E.FilePath         = InFilePath;
        E.Line             = InLine;
        E.Column           = InColumn;
        E.TargetNamespace  = InTargetNamespace;
        E.FunctionName     = InFunctionName;
        E.ArgsList         = InArgsList;
        return E;
    }

    auto Count_Occurrences(
        const FString& InHaystack,
        const FString& InNeedle) -> int32
    {
        auto Cursor = 0;
        auto Hits   = 0;
        while ((Cursor = InHaystack.Find(InNeedle, ESearchCase::CaseSensitive, ESearchDir::FromStart, Cursor)) != INDEX_NONE)
        {
            ++Hits;
            Cursor += InNeedle.Len();
        }
        return Hits;
    }

    // Extracts every emitted overload DECLARATION line from a stub file's
    // contents — the `    F<X>_SpawnParams Params(<params>)` lines AS would
    // collide on during namespace-merge. Comment lines (`// Target:`,
    // end-markers) reference `::Params(` without a leading space and are
    // excluded by the `" Params("` needle; `return F<X>_SpawnParams();`
    // lines don't contain the needle at all.
    auto Extract_OverloadDeclLines(
        const FString& InStubContents) -> TArray<FString>
    {
        auto Lines = TArray<FString>{};
        InStubContents.ParseIntoArrayLines(Lines, /*InCullEmpty=*/true);

        auto Out = TArray<FString>{};
        for (const auto& Line : Lines)
        {
            if (Line.TrimStart().StartsWith(TEXT("//")))
            { continue; }
            if (Line.Contains(TEXT(" Params("), ESearchCase::CaseSensitive))
            { Out.Add(Line.TrimStartAndEnd()); }
        }
        return Out;
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Derive_SpawnParamsStructName: convention U<X> -> F<X>_SpawnParams.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StubSynthesizer_DeriveStructName,
    "CkAngelscriptGenerator.UnitTests.StubSynthesizer.DeriveStructName",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StubSynthesizer_DeriveStructName::RunTest(const FString&)
{
    TestEqual(TEXT("UBb_X -> FBb_X_SpawnParams"),
        FCkAsStubSynthesizer::Derive_SpawnParamsStructName(TEXT("UBb_DeliveryTruck_EntityScript")),
        FString{TEXT("FBb_DeliveryTruck_EntityScript_SpawnParams")});

    TestEqual(TEXT("UCk_X -> FCk_X_SpawnParams"),
        FCkAsStubSynthesizer::Derive_SpawnParamsStructName(TEXT("UCk_Foo_EntityScript")),
        FString{TEXT("FCk_Foo_EntityScript_SpawnParams")});

    TestEqual(TEXT("empty -> empty"),
        FCkAsStubSynthesizer::Derive_SpawnParamsStructName(FString{}),
        FString{});

    TestEqual(TEXT("non-U prefix -> empty (defensive)"),
        FCkAsStubSynthesizer::Derive_SpawnParamsStructName(TEXT("assets")),
        FString{});

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Build_EntityScriptParamsStub: no-arg variant — most common case (no
// ExposeOnSpawn properties on the entity script).
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StubSynthesizer_Build_NoArg,
    "CkAngelscriptGenerator.UnitTests.StubSynthesizer.Build_NoArg",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StubSynthesizer_Build_NoArg::RunTest(const FString&)
{
    const auto Error = Make_ParsedError(TEXT("UBb_Foo_EntityScript"), TEXT("Params"), TEXT(""));
    const auto Stub  = FCkAsStubSynthesizer::Build_EntityScriptParamsStub(Error, /*InEmitStruct=*/true);

    TestTrue(TEXT("not empty"),         NOT Stub.IsEmpty());
    TestTrue(TEXT("has marker"),        Stub.Contains(FCkAsStubSynthesizer::Get_MarkerComment()));
    TestTrue(TEXT("has USTRUCT()"),     Stub.Contains(TEXT("USTRUCT()")));
    TestTrue(TEXT("has struct decl"),   Stub.Contains(TEXT("struct FBb_Foo_EntityScript_SpawnParams")));
    TestTrue(TEXT("has namespace"),     Stub.Contains(TEXT("namespace UBb_Foo_EntityScript")));
    TestTrue(TEXT("has Params()"),      Stub.Contains(TEXT("Params()")));
    TestTrue(TEXT("returns default"),   Stub.Contains(TEXT("return FBb_Foo_EntityScript_SpawnParams();")));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Build_EntityScriptParamsStub: single typed argument with "const " stripped
// from the emission to match the real generator's parameter shape.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StubSynthesizer_Build_TypedArg_StripsConst,
    "CkAngelscriptGenerator.UnitTests.StubSynthesizer.Build_TypedArg_StripsConst",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StubSynthesizer_Build_TypedArg_StripsConst::RunTest(const FString&)
{
    const auto Error = Make_ParsedError(TEXT("UBb_DeliveryTruck_EntityScript"), TEXT("Params"), TEXT("const FTransform"));
    const auto Stub  = FCkAsStubSynthesizer::Build_EntityScriptParamsStub(Error, /*InEmitStruct=*/false);

    TestTrue(TEXT("emits Params(FTransform Arg0)"),                   Stub.Contains(TEXT("Params(FTransform Arg0)")));
    TestFalse(TEXT("param-decl does NOT include 'const'"),            Stub.Contains(TEXT("const FTransform Arg0")));
    TestFalse(TEXT("does NOT emit USTRUCT (suppressed)"),             Stub.Contains(TEXT("USTRUCT()")));
    TestTrue(TEXT("references derived struct"),                       Stub.Contains(TEXT("FBb_DeliveryTruck_EntityScript_SpawnParams")));
    // The diagnostic comment "// Target: <NS>::Params(const FTransform)" preserves
    // the original error string verbatim for forensic context; only the parameter
    // declaration itself is canonicalized to match the real generator's shape.

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Build_EntityScriptParamsStub: a 'T&' lvalue-spelled arg is emitted by-value
// — matching the real generator's parameter shape and preventing a dueling
// 'T'/'T&' overload pair ("Multiple matching signatures" wedge).
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StubSynthesizer_Build_TypedArg_StripsRefMarker,
    "CkAngelscriptGenerator.UnitTests.StubSynthesizer.Build_TypedArg_StripsRefMarker",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StubSynthesizer_Build_TypedArg_StripsRefMarker::RunTest(const FString&)
{
    const auto Error = Make_ParsedError(
        TEXT("UBb_RefArgs_EntityScript"), TEXT("Params"),
        TEXT("UStaticMeshComponent&, FCk_Handle_CheckoutCounter, const FTransform&"));
    const auto Stub = FCkAsStubSynthesizer::Build_EntityScriptParamsStub(Error, /*InEmitStruct=*/false);

    TestTrue(TEXT("emits all params by value"),
        Stub.Contains(TEXT("Params(UStaticMeshComponent Arg0, FCk_Handle_CheckoutCounter Arg1, FTransform Arg2)")));
    TestFalse(TEXT("no '&' in the emitted parameter list"),
        Stub.Contains(TEXT("& Arg")));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Build_EntityScriptParamsStub: multi-arg signature.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StubSynthesizer_Build_MultiArg,
    "CkAngelscriptGenerator.UnitTests.StubSynthesizer.Build_MultiArg",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StubSynthesizer_Build_MultiArg::RunTest(const FString&)
{
    const auto Error = Make_ParsedError(
        TEXT("UBb_Multi_EntityScript"),
        TEXT("Params"),
        TEXT("const FTransform, UClass, int32"));

    const auto Stub = FCkAsStubSynthesizer::Build_EntityScriptParamsStub(Error, /*InEmitStruct=*/false);
    TestTrue(TEXT("emits all three positional args"),
        Stub.Contains(TEXT("Params(FTransform Arg0, UClass Arg1, int32 Arg2)")));
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Wrong-kind error rejects: the synthesizer is for NoMatchingSignatures only.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StubSynthesizer_Build_WrongKind_ReturnsEmpty,
    "CkAngelscriptGenerator.UnitTests.StubSynthesizer.Build_WrongKind_ReturnsEmpty",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StubSynthesizer_Build_WrongKind_ReturnsEmpty::RunTest(const FString&)
{
    auto Wrong = FCk_AsParsedError{};
    Wrong.Kind              = ECk_AsParsedError_Kind::IdentifierNotADataType;
    Wrong.MissingIdentifier = TEXT("FCk_Handle_CheckoutCounter");

    TestEqual(TEXT("IdentifierNotADataType -> empty stub"),
        FCkAsStubSynthesizer::Build_EntityScriptParamsStub(Wrong, /*InEmitStruct=*/true),
        FString{});

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Has_SpawnParamsStruct: present / absent.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StubSynthesizer_HasSpawnParamsStruct,
    "CkAngelscriptGenerator.UnitTests.StubSynthesizer.HasSpawnParamsStruct",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StubSynthesizer_HasSpawnParamsStruct::RunTest(const FString&)
{
    const auto Contents = FString{TEXT(
        "USTRUCT()\n"
        "struct FBb_Foo_EntityScript_SpawnParams\n"
        "{\n"
        "    UPROPERTY()\n"
        "    FTransform SpawnTransform;\n"
        "}\n"
        "namespace UBb_Foo_EntityScript { FBb_Foo_EntityScript_SpawnParams Params() { return FBb_Foo_EntityScript_SpawnParams(); } }\n")};

    TestTrue(TEXT("present"),
        FCkAsStubSynthesizer::Has_SpawnParamsStruct(Contents, TEXT("FBb_Foo_EntityScript_SpawnParams")));

    TestFalse(TEXT("absent"),
        FCkAsStubSynthesizer::Has_SpawnParamsStruct(Contents, TEXT("FBb_DoesNotExist_SpawnParams")));

    TestFalse(TEXT("empty name -> false"),
        FCkAsStubSynthesizer::Has_SpawnParamsStruct(Contents, FString{}));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Find_TargetFile_ByContent: unique match, no match, ambiguous match.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StubSynthesizer_FindTargetFile_ByContent,
    "CkAngelscriptGenerator.UnitTests.StubSynthesizer.FindTargetFile_ByContent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StubSynthesizer_FindTargetFile_ByContent::RunTest(const FString&)
{
    // Set up temp candidate files.
    const auto TempRoot   = FPaths::ProjectIntermediateDir() / TEXT("CkStubSynthTest");
    const auto FileA      = TempRoot / TEXT("PluginA_EntitySpawnParams.as");
    const auto FileB      = TempRoot / TEXT("PluginB_EntitySpawnParams.as");
    const auto FileC      = TempRoot / TEXT("PluginC_EntitySpawnParams.as");
    IFileManager::Get().MakeDirectory(*TempRoot, /*Tree=*/true);
    FFileHelper::SaveStringToFile(FString{TEXT("// PluginA — references UBb_Foo_EntityScript here\n")}, *FileA);
    FFileHelper::SaveStringToFile(FString{TEXT("// PluginB — no relevant content\n")},                  *FileB);
    FFileHelper::SaveStringToFile(FString{TEXT("// PluginC — also references UBb_Foo_EntityScript\n")}, *FileC);

    // Single-candidate hit (PluginA only).
    {
        const auto Match = FCkAsStubSynthesizer::Find_TargetFile_ByContent(
            TEXT("UBb_Foo_EntityScript"),
            {FileA, FileB});
        TestEqual(TEXT("single match -> that path"), Match, FileA);
    }

    // No-candidate hit.
    {
        const auto Match = FCkAsStubSynthesizer::Find_TargetFile_ByContent(
            TEXT("UBb_DoesNotExist_EntityScript"),
            {FileA, FileB, FileC});
        TestEqual(TEXT("no match -> empty"), Match, FString{});
    }

    // Ambiguous hit (PluginA and PluginC both reference) -> defensive empty.
    {
        const auto Match = FCkAsStubSynthesizer::Find_TargetFile_ByContent(
            TEXT("UBb_Foo_EntityScript"),
            {FileA, FileB, FileC});
        TestEqual(TEXT("ambiguous match -> empty (defensive)"), Match, FString{});
    }

    // Cleanup.
    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Inject_EntityScriptParamsStub: end-to-end with a temp fixture that mimics
// the post-corruption shape (struct intact, namespace mangled).
// Verifies the original contents are preserved and the stub is appended with
// the namespace-only variant (no second struct definition, since the struct
// is already in the file).
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StubSynthesizer_Inject_EndToEnd,
    "CkAngelscriptGenerator.UnitTests.StubSynthesizer.Inject_EndToEnd",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StubSynthesizer_Inject_EndToEnd::RunTest(const FString&)
{
    const auto TempRoot = FPaths::ProjectIntermediateDir() / TEXT("CkStubSynthTest_Inject");
    const auto FixtureFile = TempRoot / TEXT("BusterBlock_EntitySpawnParams.as");
    const auto ExpectedStubFile = TempRoot / TEXT("_StubRecovery_BusterBlock_EntitySpawnParams.as");
    IFileManager::Get().MakeDirectory(*TempRoot, /*Tree=*/true);

    // Pre-corruption fixture content. The struct is intact, the namespace has
    // been renamed (mimicking corrupt_for_rev7_test.bat).
    const auto Original = FString{TEXT(
        "// Auto-generated EntityScript spawn-params — DO NOT EDIT.\n"
        "\n"
        "USTRUCT()\n"
        "struct FBb_DeliveryTruck_EntityScript_SpawnParams\n"
        "{\n"
        "}\n"
        "\n"
        "namespace UBb_DeliveryTruck_EntityScript_REV7_CORRUPTED\n"
        "{\n"
        "    FBb_DeliveryTruck_EntityScript_SpawnParams Params(FTransform InSpawnTransform)\n"
        "    {\n"
        "        return FBb_DeliveryTruck_EntityScript_SpawnParams();\n"
        "    }\n"
        "}\n")};
    FFileHelper::SaveStringToFile(Original, *FixtureFile,
        FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    const auto Error = Make_ParsedError(
        TEXT("UBb_DeliveryTruck_EntityScript"),
        TEXT("Params"),
        TEXT("const FTransform"));

    const auto Result = FCkAsStubSynthesizer::Inject_EntityScriptParamsStub(Error, {FixtureFile});

    TestTrue(TEXT("Success"),                Result.Success);
    TestEqual(TEXT("Target = sibling stub"), Result.TargetFilePath, ExpectedStubFile);
    TestFalse(TEXT("InjectedBlock non-empty"), Result.InjectedBlock.IsEmpty());

    // The canonical fixture file must be byte-identical to its pre-call state.
    auto CanonicalAfter = FString{};
    FFileHelper::LoadFileToString(CanonicalAfter, *FixtureFile);
    TestEqual(TEXT("canonical fixture untouched"), CanonicalAfter, Original);

    // The sibling stub file must exist and contain the injected block + header.
    auto StubAfter = FString{};
    TestTrue(TEXT("sibling stub written"), FFileHelper::LoadFileToString(StubAfter, *ExpectedStubFile));
    TestTrue(TEXT("stub starts with recovery header"),
        StubAfter.Contains(TEXT("AUTO-GENERATED RECOVERY STUBS")));
    TestTrue(TEXT("stub contains injected block"), StubAfter.Contains(Result.InjectedBlock));
    TestTrue(TEXT("stub has correct namespace"),
        StubAfter.Contains(TEXT("namespace UBb_DeliveryTruck_EntityScript")));
    TestTrue(TEXT("stub has correct Params signature"),
        StubAfter.Contains(TEXT("Params(FTransform Arg0)")));
    TestFalse(TEXT("stub does NOT re-emit struct (canonical already has it)"),
        Result.InjectedBlock.Contains(TEXT("USTRUCT()")));
    TestTrue(TEXT("marker comment is present in stub"),
        StubAfter.Contains(FCkAsStubSynthesizer::Get_MarkerComment()));

    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Inject_EntityScriptParamsStub: missing-struct path — when no struct of the
// expected name is present, the injected block must include a USTRUCT stub.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StubSynthesizer_Inject_MissingStruct,
    "CkAngelscriptGenerator.UnitTests.StubSynthesizer.Inject_MissingStruct",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StubSynthesizer_Inject_MissingStruct::RunTest(const FString&)
{
    const auto TempRoot = FPaths::ProjectIntermediateDir() / TEXT("CkStubSynthTest_MissingStruct");
    const auto FixtureFile = TempRoot / TEXT("BusterBlock_EntitySpawnParams.as");
    const auto ExpectedStubFile = TempRoot / TEXT("_StubRecovery_BusterBlock_EntitySpawnParams.as");
    IFileManager::Get().MakeDirectory(*TempRoot, /*Tree=*/true);

    // Fixture: file references the namespace by name (so Find_TargetFile_ByContent
    // can locate it) but the struct definition is absent.
    const auto Original = FString{TEXT(
        "// Auto-generated EntityScript spawn-params — DO NOT EDIT.\n"
        "// (No FBb_Orphan_EntityScript_SpawnParams struct present.)\n"
        "// Reference: UBb_Orphan_EntityScript appears as a string in this comment.\n")};
    FFileHelper::SaveStringToFile(Original, *FixtureFile,
        FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    const auto Error = Make_ParsedError(TEXT("UBb_Orphan_EntityScript"), TEXT("Params"), TEXT(""));
    const auto Result = FCkAsStubSynthesizer::Inject_EntityScriptParamsStub(Error, {FixtureFile});

    TestTrue(TEXT("Success"),                                  Result.Success);
    TestEqual(TEXT("Target = sibling stub"),                   Result.TargetFilePath, ExpectedStubFile);
    TestTrue(TEXT("injected block includes USTRUCT (struct missing)"),
        Result.InjectedBlock.Contains(TEXT("USTRUCT()")));
    TestTrue(TEXT("injected block includes struct decl"),
        Result.InjectedBlock.Contains(TEXT("struct FBb_Orphan_EntityScript_SpawnParams")));

    // Canonical untouched.
    auto CanonicalAfter = FString{};
    FFileHelper::LoadFileToString(CanonicalAfter, *FixtureFile);
    TestEqual(TEXT("canonical untouched"), CanonicalAfter, Original);

    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Inject_EntityScriptParamsStub: brand-new namespace (no content match) anchors
// against the caller .as file's nearest .uproject ancestor — the project-side
// case (BB-style layout: caller under <ProjectRoot>/Script/...).
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StubSynthesizer_Inject_AnchorsByCallerPath_Project,
    "CkAngelscriptGenerator.UnitTests.StubSynthesizer.Inject_AnchorsByCallerPath_Project",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StubSynthesizer_Inject_AnchorsByCallerPath_Project::RunTest(const FString&)
{
    // Normalize to absolute up-front so the expected path matches what
    // Anchor_ByCallerAsPath returns (it canonicalizes via ConvertRelativePathToFull).
    const auto TempRoot = FPaths::ConvertRelativePathToFull(
        FPaths::ProjectIntermediateDir() / TEXT("CkStubSynthTest_AnchorProject"));
    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);
    IFileManager::Get().MakeDirectory(*(TempRoot / TEXT("Script/Tests")), /*Tree=*/true);

    const auto ProjectName    = FString{TEXT("FakeProject")};
    const auto UProjectFile   = TempRoot / (ProjectName + TEXT(".uproject"));
    const auto CallerAsFile   = TempRoot / TEXT("Script/Tests/Probe.as");
    const auto ExpectedStub   = TempRoot / TEXT("Script/Generated") / (FString{TEXT("_StubRecovery_")} + ProjectName + TEXT("_EntitySpawnParams.as"));

    FFileHelper::SaveStringToFile(FString{TEXT("{}")},                    *UProjectFile);
    FFileHelper::SaveStringToFile(FString{TEXT("// probe .as file\n")}, *CallerAsFile);

    auto Error = Make_ParsedError(
        TEXT("UProbeAdditive_EntityScript"), TEXT("Params"), TEXT(""),
        *CallerAsFile, /*InLine=*/1, /*InColumn=*/1);

    const auto Result = FCkAsStubSynthesizer::Inject_EntityScriptParamsStub(Error, /*InCandidateFilePaths=*/{});

    TestTrue(TEXT("Success"), Result.Success);
    TestEqual(TEXT("Target = sibling under project Script/Generated"),
        Result.TargetFilePath, ExpectedStub);
    TestTrue(TEXT("brand-new -> emits USTRUCT"),
        Result.InjectedBlock.Contains(TEXT("USTRUCT()")));
    TestTrue(TEXT("brand-new -> emits namespace"),
        Result.InjectedBlock.Contains(TEXT("namespace UProbeAdditive_EntityScript")));

    auto StubAfter = FString{};
    TestTrue(TEXT("sibling stub written to disk"), FFileHelper::LoadFileToString(StubAfter, *ExpectedStub));
    TestTrue(TEXT("recovery header banner present"),
        StubAfter.Contains(TEXT("AUTO-GENERATED RECOVERY STUBS")));

    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Inject_EntityScriptParamsStub: brand-new namespace anchors against the
// caller .as file's nearest .uplugin ancestor — the plugin-side case.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StubSynthesizer_Inject_AnchorsByCallerPath_Plugin,
    "CkAngelscriptGenerator.UnitTests.StubSynthesizer.Inject_AnchorsByCallerPath_Plugin",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StubSynthesizer_Inject_AnchorsByCallerPath_Plugin::RunTest(const FString&)
{
    const auto TempRoot   = FPaths::ConvertRelativePathToFull(
        FPaths::ProjectIntermediateDir() / TEXT("CkStubSynthTest_AnchorPlugin"));
    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);

    const auto PluginName = FString{TEXT("FooPlugin")};
    const auto PluginRoot = TempRoot / TEXT("Plugins") / PluginName;
    const auto UPluginFile  = PluginRoot / (PluginName + TEXT(".uplugin"));
    const auto CallerAsFile = PluginRoot / TEXT("Script/Tests/PluginProbe.as");
    const auto ExpectedStub = PluginRoot / TEXT("Script/Generated") / (FString{TEXT("_StubRecovery_")} + PluginName + TEXT("_EntitySpawnParams.as"));

    IFileManager::Get().MakeDirectory(*FPaths::GetPath(CallerAsFile), /*Tree=*/true);
    FFileHelper::SaveStringToFile(FString{TEXT("{}")},                    *UPluginFile);
    FFileHelper::SaveStringToFile(FString{TEXT("// plugin probe .as\n")}, *CallerAsFile);

    auto Error = Make_ParsedError(
        TEXT("UFooPlugin_BrandNew_EntityScript"), TEXT("Params"), TEXT(""),
        *CallerAsFile, /*InLine=*/1, /*InColumn=*/1);

    const auto Result = FCkAsStubSynthesizer::Inject_EntityScriptParamsStub(Error, /*InCandidateFilePaths=*/{});

    TestTrue(TEXT("Success"), Result.Success);
    TestEqual(TEXT("Target = sibling under plugin Script/Generated"),
        Result.TargetFilePath, ExpectedStub);
    TestTrue(TEXT("written to disk"),
        IFileManager::Get().FileExists(*ExpectedStub));

    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Inject_EntityScriptParamsStub: no candidate match AND no .uplugin/.uproject
// ancestor — must fail cleanly with a descriptive error. Pins the defensive
// failure path so a regression doesn't silently write a stub at filesystem root.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StubSynthesizer_Inject_AnchorsByCallerPath_NoManifest_Fails,
    "CkAngelscriptGenerator.UnitTests.StubSynthesizer.Inject_AnchorsByCallerPath_NoManifest_Fails",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StubSynthesizer_Inject_AnchorsByCallerPath_NoManifest_Fails::RunTest(const FString&)
{
    // Caller path with no manifest ancestor anywhere on the chain. Use a path
    // intentionally outside the engine + project trees — UE Saved/ dirs never
    // contain a .uplugin/.uproject, and we don't traverse into them under
    // normal authoring conditions.
    const auto BogusCaller = FString{TEXT("//NoSuch/RootOnly/Probe.as")};

    auto Error = Make_ParsedError(
        TEXT("UNeverHeardOf_EntityScript"), TEXT("Params"), TEXT(""),
        *BogusCaller, /*InLine=*/1, /*InColumn=*/1);

    const auto Result = FCkAsStubSynthesizer::Inject_EntityScriptParamsStub(Error, /*InCandidateFilePaths=*/{});

    TestFalse(TEXT("Success = false"), Result.Success);
    TestFalse(TEXT("ErrorMessage populated"), Result.ErrorMessage.IsEmpty());
    TestTrue(TEXT("error mentions missing manifest ancestor"),
        Result.ErrorMessage.Contains(TEXT(".uplugin")));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Anchor_ByCallerAsPath: direct unit test of the new helper across the three
// shape cases. Cheaper / faster signal than going through Inject_*.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StubSynthesizer_AnchorByCallerAsPath_Direct,
    "CkAngelscriptGenerator.UnitTests.StubSynthesizer.AnchorByCallerAsPath_Direct",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StubSynthesizer_AnchorByCallerAsPath_Direct::RunTest(const FString&)
{
    const auto TempRoot = FPaths::ConvertRelativePathToFull(
        FPaths::ProjectIntermediateDir() / TEXT("CkStubSynthTest_AnchorDirect"));
    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);

    // --- Project case: .uproject at TempRoot, caller a few dirs below.
    {
        const auto ProjectName  = FString{TEXT("FakeProj")};
        const auto UProjectFile = TempRoot / (ProjectName + TEXT(".uproject"));
        const auto Caller       = TempRoot / TEXT("Script/Probe.as");
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(Caller), /*Tree=*/true);
        FFileHelper::SaveStringToFile(FString{TEXT("{}")}, *UProjectFile);

        const auto Got      = FCkAsStubSynthesizer::Anchor_ByCallerAsPath(Caller);
        const auto Expected = TempRoot / TEXT("Script/Generated") / (ProjectName + TEXT("_EntitySpawnParams.as"));
        TestEqual(TEXT("project anchor"), Got, Expected);
    }

    // --- Plugin case: .uplugin nested under TempRoot (and the project case
    // above already left a .uproject at TempRoot, so this also pins the
    // "plugin wins when both ancestors exist" guarantee).
    {
        const auto PluginName = FString{TEXT("Bar")};
        const auto PluginRoot = TempRoot / TEXT("Plugins") / PluginName;
        const auto UPluginFile = PluginRoot / (PluginName + TEXT(".uplugin"));
        const auto Caller     = PluginRoot / TEXT("Script/Nested/Probe.as");
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(Caller), /*Tree=*/true);
        FFileHelper::SaveStringToFile(FString{TEXT("{}")}, *UPluginFile);

        const auto Got      = FCkAsStubSynthesizer::Anchor_ByCallerAsPath(Caller);
        const auto Expected = PluginRoot / TEXT("Script/Generated") / (PluginName + TEXT("_EntitySpawnParams.as"));
        TestEqual(TEXT("plugin anchor (wins over project ancestor)"), Got, Expected);
    }

    // --- Empty input -> empty output.
    {
        TestEqual(TEXT("empty caller -> empty"),
            FCkAsStubSynthesizer::Anchor_ByCallerAsPath(FString{}), FString{});
    }

    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Derive_StubSiblingPath: filename gets `_StubRecovery_` prefix in same dir.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StubSynthesizer_DeriveStubSiblingPath,
    "CkAngelscriptGenerator.UnitTests.StubSynthesizer.DeriveStubSiblingPath",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StubSynthesizer_DeriveStubSiblingPath::RunTest(const FString&)
{
    const auto Canonical = FString{TEXT("D:/Repos/BB/Script/Generated/BusterBlock_EntitySpawnParams.as")};
    const auto Expected  = FString{TEXT("D:/Repos/BB/Script/Generated/_StubRecovery_BusterBlock_EntitySpawnParams.as")};

    TestEqual(TEXT("sibling matches expected path"),
        FCkAsStubSynthesizer::Derive_StubSiblingPath(Canonical), Expected);

    TestEqual(TEXT("empty input -> empty"),
        FCkAsStubSynthesizer::Derive_StubSiblingPath(FString{}), FString{});

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Stub-file recovery header is non-empty and identifies the file as auto-generated.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StubSynthesizer_StubFileHeader,
    "CkAngelscriptGenerator.UnitTests.StubSynthesizer.StubFileHeader",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StubSynthesizer_StubFileHeader::RunTest(const FString&)
{
    const auto Header = FCkAsStubSynthesizer::Get_StubFileHeader();
    TestFalse(TEXT("not empty"), Header.IsEmpty());
    TestTrue(TEXT("contains AUTO-GENERATED RECOVERY STUBS"),
        Header.Contains(TEXT("AUTO-GENERATED RECOVERY STUBS")));
    TestTrue(TEXT("notes GITIGNORED status"), Header.Contains(TEXT("GITIGNORED")));
    TestTrue(TEXT("notes self-cleans"),       Header.Contains(TEXT("self-cleans")));
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Accumulating append: two consecutive Inject calls against the same plugin's
// canonical file land both stubs in the same sibling file with header written
// only once.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StubSynthesizer_Inject_Accumulating,
    "CkAngelscriptGenerator.UnitTests.StubSynthesizer.Inject_Accumulating",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StubSynthesizer_Inject_Accumulating::RunTest(const FString&)
{
    const auto TempRoot         = FPaths::ProjectIntermediateDir() / TEXT("CkStubSynthTest_Accum");
    const auto FixtureFile      = TempRoot / TEXT("BusterBlock_EntitySpawnParams.as");
    const auto ExpectedStubFile = TempRoot / TEXT("_StubRecovery_BusterBlock_EntitySpawnParams.as");
    IFileManager::Get().MakeDirectory(*TempRoot, /*Tree=*/true);

    // Canonical references two namespaces but has no struct for either —
    // each Inject should emit its own USTRUCT + namespace.
    const auto Original = FString{TEXT(
        "// Pre-existing canonical.\n"
        "// References UBb_Alpha_EntityScript and UBb_Beta_EntityScript as strings.\n")};
    FFileHelper::SaveStringToFile(Original, *FixtureFile,
        FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    const auto ErrorA = Make_ParsedError(TEXT("UBb_Alpha_EntityScript"), TEXT("Params"), TEXT(""));
    const auto ErrorB = Make_ParsedError(TEXT("UBb_Beta_EntityScript"),  TEXT("Params"), TEXT(""));

    const auto ResultA = FCkAsStubSynthesizer::Inject_EntityScriptParamsStub(ErrorA, {FixtureFile});
    TestTrue(TEXT("first inject succeeded"), ResultA.Success);

    const auto ResultB = FCkAsStubSynthesizer::Inject_EntityScriptParamsStub(ErrorB, {FixtureFile});
    TestTrue(TEXT("second inject succeeded"), ResultB.Success);

    TestEqual(TEXT("both target the same sibling file"),
        ResultA.TargetFilePath, ResultB.TargetFilePath);
    TestEqual(TEXT("sibling path matches expected"),
        ResultA.TargetFilePath, ExpectedStubFile);

    auto Stub = FString{};
    TestTrue(TEXT("sibling readable"), FFileHelper::LoadFileToString(Stub, *ExpectedStubFile));

    TestTrue(TEXT("contains first injected block"),  Stub.Contains(ResultA.InjectedBlock));
    TestTrue(TEXT("contains second injected block"), Stub.Contains(ResultB.InjectedBlock));
    TestTrue(TEXT("header for both alpha"),  Stub.Contains(TEXT("namespace UBb_Alpha_EntityScript")));
    TestTrue(TEXT("header for both beta"),   Stub.Contains(TEXT("namespace UBb_Beta_EntityScript")));

    // Header banner appears exactly once (count the unique banner text).
    auto Cursor   = 0;
    auto HitCount = 0;
    const auto Needle = FString{TEXT("AUTO-GENERATED RECOVERY STUBS")};
    while ((Cursor = Stub.Find(Needle, ESearchCase::IgnoreCase, ESearchDir::FromStart, Cursor)) != INDEX_NONE)
    {
        ++HitCount;
        Cursor += Needle.Len();
    }
    TestEqual(TEXT("header banner appears exactly once"), HitCount, 1);

    // Canonical untouched.
    auto CanonicalAfter = FString{};
    FFileHelper::LoadFileToString(CanonicalAfter, *FixtureFile);
    TestEqual(TEXT("canonical untouched after both injects"), CanonicalAfter, Original);

    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Dedup on same accessor: regression for the duplicate-function collision
// (user-reported 2026-05-14). Two Inject calls for the same (namespace,
// function) must NOT produce a duplicate `Params(...)` declaration in the
// sibling — AS namespace-merge would reject it with "A function with the
// same name and parameters already exists" at next compile. The per-accessor
// dedup gate in Inject_EntityScriptParamsStub scans existing sibling content
// for the unique `// End synthesized stub for <NS>::<FUNC>(<args>)` marker
// line and short-circuits the append when present.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StubSynthesizer_Inject_DedupOnSameAccessor,
    "CkAngelscriptGenerator.UnitTests.StubSynthesizer.Inject_DedupOnSameAccessor",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StubSynthesizer_Inject_DedupOnSameAccessor::RunTest(const FString&)
{
    const auto TempRoot         = FPaths::ProjectIntermediateDir() / TEXT("CkStubSynthTest_Dedup");
    const auto FixtureFile      = TempRoot / TEXT("BusterBlock_EntitySpawnParams.as");
    const auto ExpectedStubFile = TempRoot / TEXT("_StubRecovery_BusterBlock_EntitySpawnParams.as");
    IFileManager::Get().MakeDirectory(*TempRoot, /*Tree=*/true);

    const auto Original = FString{TEXT("// References UBb_Gamma_EntityScript as a string.\n")};
    FFileHelper::SaveStringToFile(Original, *FixtureFile,
        FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    const auto Error = Make_ParsedError(TEXT("UBb_Gamma_EntityScript"), TEXT("Params"), TEXT(""));

    const auto ResultA = FCkAsStubSynthesizer::Inject_EntityScriptParamsStub(Error, {FixtureFile});
    TestTrue(TEXT("first inject succeeded"), ResultA.Success);

    const auto ResultB = FCkAsStubSynthesizer::Inject_EntityScriptParamsStub(Error, {FixtureFile});
    TestTrue(TEXT("second inject (same accessor) reports success (no-op)"), ResultB.Success);

    auto Stub = FString{};
    TestTrue(TEXT("sibling readable"), FFileHelper::LoadFileToString(Stub, *ExpectedStubFile));

    // End-marker line for this accessor must appear exactly once.
    auto Cursor   = 0;
    auto HitCount = 0;
    const auto Needle = FString{TEXT("// End synthesized stub for UBb_Gamma_EntityScript::Params")};
    while ((Cursor = Stub.Find(Needle, ESearchCase::CaseSensitive, ESearchDir::FromStart, Cursor)) != INDEX_NONE)
    {
        ++HitCount;
        Cursor += Needle.Len();
    }
    TestEqual(TEXT("end-marker for the accessor appears exactly once"), HitCount, 1);

    // The Params(...) overload declaration must appear exactly once — this is
    // the AS-side identifier that would collide on namespace-merge.
    auto ParamsCursor = 0;
    auto ParamsHits   = 0;
    const auto ParamsNeedle = FString{TEXT(" Params(")};
    while ((ParamsCursor = Stub.Find(ParamsNeedle, ESearchCase::CaseSensitive, ESearchDir::FromStart, ParamsCursor)) != INDEX_NONE)
    {
        ++ParamsHits;
        ParamsCursor += ParamsNeedle.Len();
    }
    TestEqual(TEXT("Params(...) declaration appears exactly once"), ParamsHits, 1);

    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);
    return true;
}


// --------------------------------------------------------------------------------------------------------------------
// Normalize_ArgsList: strips const and & per token; distinct base types stay
// distinct. This is the canonicalization shared by stub emission, the dedup
// end-marker, and the dispatcher's convergence key.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StubSynthesizer_NormalizeArgsList,
    "CkAngelscriptGenerator.UnitTests.StubSynthesizer.NormalizeArgsList",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StubSynthesizer_NormalizeArgsList::RunTest(const FString&)
{
    TestEqual(TEXT("const + ref variants collapse to value form"),
        FCkAsStubSynthesizer::Normalize_ArgsList(
            TEXT("FTransform, FCk_Handle_CheckoutCounter&, const EBb_Role, const int, FVector")),
        FString{TEXT("FTransform, FCk_Handle_CheckoutCounter, EBb_Role, int, FVector")});

    TestEqual(TEXT("int& and const int normalize identically"),
        FCkAsStubSynthesizer::Normalize_ArgsList(TEXT("int&")),
        FCkAsStubSynthesizer::Normalize_ArgsList(TEXT("const int")));

    TestNotEqual(TEXT("distinct base types stay distinct"),
        FCkAsStubSynthesizer::Normalize_ArgsList(TEXT("int")),
        FCkAsStubSynthesizer::Normalize_ArgsList(TEXT("float")));

    TestEqual(TEXT("empty -> empty"),
        FCkAsStubSynthesizer::Normalize_ArgsList(FString{}), FString{});

    TestEqual(TEXT("nullptr placeholder maps to UObject fallback"),
        FCkAsStubSynthesizer::Normalize_ArgsList(TEXT("FTransform, <null handle>")),
        FString{TEXT("FTransform, UObject")});

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Dedup on argument-category variants: regression for the 2026-06-10
// CheckoutSettle incident. Two call sites of the SAME Params() — one passing
// a literal (error reports "const int"), one passing an lvalue (error reports
// "int&") — must produce ONE value-typed stub, not two. Two stubs differing
// only by lvalue-ness are mutually ambiguous at every call site ("Multiple
// matching signatures"), which the dispatcher cannot recognize or heal.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StubSynthesizer_Inject_DedupOnArgCategoryVariants,
    "CkAngelscriptGenerator.UnitTests.StubSynthesizer.Inject_DedupOnArgCategoryVariants",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StubSynthesizer_Inject_DedupOnArgCategoryVariants::RunTest(const FString&)
{
    const auto TempRoot         = FPaths::ProjectIntermediateDir() / TEXT("CkStubSynthTest_DedupVariants");
    const auto FixtureFile      = TempRoot / TEXT("BusterBlock_EntitySpawnParams.as");
    const auto ExpectedStubFile = TempRoot / TEXT("_StubRecovery_BusterBlock_EntitySpawnParams.as");
    IFileManager::Get().MakeDirectory(*TempRoot, /*Tree=*/true);

    const auto Original = FString{TEXT("// References UBb_Settle_EntityScript as a string.\n")};
    FFileHelper::SaveStringToFile(Original, *FixtureFile,
        FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    // Same function, two call sites: literal int vs lvalue int.
    const auto ErrorLiteral = Make_ParsedError(
        TEXT("UBb_Settle_EntityScript"), TEXT("Params"),
        TEXT("FTransform, FCk_Handle_CheckoutCounter&, const EBb_Role, const int, FVector"),
        TEXT("D:/Test/Caller.as"), 319, 9);
    const auto ErrorLvalue = Make_ParsedError(
        TEXT("UBb_Settle_EntityScript"), TEXT("Params"),
        TEXT("FTransform, FCk_Handle_CheckoutCounter&, const EBb_Role, int&, FVector"),
        TEXT("D:/Test/Caller.as"), 543, 13);

    const auto ResultA = FCkAsStubSynthesizer::Inject_EntityScriptParamsStub(ErrorLiteral, {FixtureFile});
    TestTrue(TEXT("first inject succeeded"), ResultA.Success);

    const auto ResultB = FCkAsStubSynthesizer::Inject_EntityScriptParamsStub(ErrorLvalue, {FixtureFile});
    TestTrue(TEXT("second inject (category variant) reports success (no-op)"), ResultB.Success);

    auto Stub = FString{};
    TestTrue(TEXT("sibling readable"), FFileHelper::LoadFileToString(Stub, *ExpectedStubFile));

    // Exactly one Params(...) declaration — the AS-side identifier that would
    // be ambiguous on namespace-merge if both variants landed.
    auto Cursor   = 0;
    auto HitCount = 0;
    const auto Needle = FString{TEXT(" Params(")};
    while ((Cursor = Stub.Find(Needle, ESearchCase::CaseSensitive, ESearchDir::FromStart, Cursor)) != INDEX_NONE)
    {
        ++HitCount;
        Cursor += Needle.Len();
    }
    TestEqual(TEXT("Params(...) declaration appears exactly once"), HitCount, 1);

    // The emitted parameter list is value-typed — accepts both a literal and
    // an lvalue at the call sites.
    TestTrue(TEXT("emitted param list is value-typed"),
        Stub.Contains(TEXT("Params(FTransform Arg0, FCk_Handle_CheckoutCounter Arg1, EBb_Role Arg2, int Arg3, FVector Arg4)")));
    TestFalse(TEXT("no reference-typed int param emitted"), Stub.Contains(TEXT("int& Arg3")));

    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);
    return true;
}


// --------------------------------------------------------------------------------------------------------------------
// Dedup on const-aliased args: regression for the BULK-synthesis duplicate-
// overload collision (fresh-clone boot test, 2026-06 — errors at
// _StubRecovery_BusterBlock_EntitySpawnParams.as (761:5)/(911:5)).
//
// Two call sites hit the same overload with differently-qualified arguments,
// so the parser reports two RAW args lists ("FTransform" vs "const
// FTransform"). The emitted declaration is const-stripped in both cases —
// `Params(FTransform Arg0)` — so unless the dedup key is canonicalized the
// same way, both blocks append and AS rejects the merged namespace with
// "A function with the same name and parameters already exists".
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StubSynthesizer_Inject_DedupOnConstAliasedArgs,
    "CkAngelscriptGenerator.UnitTests.StubSynthesizer.Inject_DedupOnConstAliasedArgs",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StubSynthesizer_Inject_DedupOnConstAliasedArgs::RunTest(const FString&)
{
    const auto TempRoot         = FPaths::ProjectIntermediateDir() / TEXT("CkStubSynthTest_ConstAlias");
    const auto FixtureFile      = TempRoot / TEXT("BusterBlock_EntitySpawnParams.as");
    const auto ExpectedStubFile = TempRoot / TEXT("_StubRecovery_BusterBlock_EntitySpawnParams.as");
    IFileManager::Get().MakeDirectory(*TempRoot, /*Tree=*/true);

    const auto Original = FString{TEXT("// References UBb_Delta_EntityScript as a string.\n")};
    FFileHelper::SaveStringToFile(Original, *FixtureFile,
        FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    const auto ErrorPlain   = Make_ParsedError(TEXT("UBb_Delta_EntityScript"), TEXT("Params"), TEXT("FTransform"));
    const auto ErrorConst   = Make_ParsedError(TEXT("UBb_Delta_EntityScript"), TEXT("Params"), TEXT("const FTransform"));
    const auto ErrorRef     = Make_ParsedError(TEXT("UBb_Delta_EntityScript"), TEXT("Params"), TEXT("FTransform&"));

    const auto ResultA = FCkAsStubSynthesizer::Inject_EntityScriptParamsStub(ErrorPlain, {FixtureFile});
    TestTrue(TEXT("plain-args inject succeeded"), ResultA.Success);

    const auto ResultB = FCkAsStubSynthesizer::Inject_EntityScriptParamsStub(ErrorConst, {FixtureFile});
    TestTrue(TEXT("const-args inject (same canonical overload) reports success (no-op)"), ResultB.Success);

    const auto ResultC = FCkAsStubSynthesizer::Inject_EntityScriptParamsStub(ErrorRef, {FixtureFile});
    TestTrue(TEXT("ref-args inject (lvalue 'T&' spelling) reports success (no-op)"), ResultC.Success);

    auto Stub = FString{};
    TestTrue(TEXT("sibling readable"), FFileHelper::LoadFileToString(Stub, *ExpectedStubFile));

    TestEqual(TEXT("emitted Params(FTransform Arg0) declaration appears exactly once"),
        Count_Occurrences(Stub, TEXT(" Params(FTransform Arg0)")), 1);
    TestFalse(TEXT("no by-ref overload emitted (dueling 'T&' stub)"),
        Stub.Contains(TEXT("FTransform&")));

    const auto DeclLines = Extract_OverloadDeclLines(Stub);
    const auto UniqueDecls = TSet<FString>{DeclLines};
    TestEqual(TEXT("no duplicate overload declarations in sibling"),
        UniqueDecls.Num(), DeclLines.Num());

    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Bulk synthesis sweep: the wholesale-missing-ESP case a fresh clone creates.
// Many accessors across several namespaces synthesize into one sibling, with
// the whole batch re-fired a second time (a second modal-tick drain after a
// still-failing compile) including const-aliased variants. Every emitted
// overload declaration must appear exactly once — any duplicate wedges the
// next compile.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StubSynthesizer_Inject_BulkSynthesis_NoDuplicateOverloads,
    "CkAngelscriptGenerator.UnitTests.StubSynthesizer.Inject_BulkSynthesis_NoDuplicateOverloads",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StubSynthesizer_Inject_BulkSynthesis_NoDuplicateOverloads::RunTest(const FString&)
{
    const auto TempRoot         = FPaths::ProjectIntermediateDir() / TEXT("CkStubSynthTest_Bulk");
    const auto FixtureFile      = TempRoot / TEXT("BusterBlock_EntitySpawnParams.as");
    const auto ExpectedStubFile = TempRoot / TEXT("_StubRecovery_BusterBlock_EntitySpawnParams.as");
    IFileManager::Get().MakeDirectory(*TempRoot, /*Tree=*/true);

    const auto Original = FString{TEXT(
        "// References UBb_BulkA_EntityScript, UBb_BulkB_EntityScript,\n"
        "// UBb_BulkC_EntityScript, UBb_BulkD_EntityScript as strings.\n")};
    FFileHelper::SaveStringToFile(Original, *FixtureFile,
        FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    // Drain 1: distinct namespaces and overload shapes, including a
    // const-aliased pair within the batch (BulkB).
    const auto Drain1 = TArray<FCk_AsParsedError>{
        Make_ParsedError(TEXT("UBb_BulkA_EntityScript"), TEXT("Params"), TEXT("")),
        Make_ParsedError(TEXT("UBb_BulkB_EntityScript"), TEXT("Params"), TEXT("const FTransform")),
        Make_ParsedError(TEXT("UBb_BulkB_EntityScript"), TEXT("Params"), TEXT("FTransform")),
        Make_ParsedError(TEXT("UBb_BulkC_EntityScript"), TEXT("Params"), TEXT("FTransform, const int32")),
        Make_ParsedError(TEXT("UBb_BulkD_EntityScript"), TEXT("Params"), TEXT("const UClass, bool")),
    };

    // Drain 2: the same batch re-fires (compile still failing), with the
    // const-qualifications flipped on the multi-arg signatures.
    const auto Drain2 = TArray<FCk_AsParsedError>{
        Make_ParsedError(TEXT("UBb_BulkA_EntityScript"), TEXT("Params"), TEXT("")),
        Make_ParsedError(TEXT("UBb_BulkB_EntityScript"), TEXT("Params"), TEXT("FTransform")),
        Make_ParsedError(TEXT("UBb_BulkC_EntityScript"), TEXT("Params"), TEXT("const FTransform, int32")),
        Make_ParsedError(TEXT("UBb_BulkD_EntityScript"), TEXT("Params"), TEXT("UClass, bool")),
    };

    for (const auto& Error : Drain1)
    {
        TestTrue(TEXT("drain-1 inject succeeded"),
            FCkAsStubSynthesizer::Inject_EntityScriptParamsStub(Error, {FixtureFile}).Success);
    }
    for (const auto& Error : Drain2)
    {
        TestTrue(TEXT("drain-2 inject succeeded (no-op or append)"),
            FCkAsStubSynthesizer::Inject_EntityScriptParamsStub(Error, {FixtureFile}).Success);
    }

    auto Stub = FString{};
    TestTrue(TEXT("sibling readable"), FFileHelper::LoadFileToString(Stub, *ExpectedStubFile));

    const auto DeclLines   = Extract_OverloadDeclLines(Stub);
    const auto UniqueDecls = TSet<FString>{DeclLines};
    TestEqual(TEXT("every overload declaration appears exactly once"),
        UniqueDecls.Num(), DeclLines.Num());

    // 4 distinct canonical overloads total: A(), B(FTransform),
    // C(FTransform, int32), D(UClass, bool) — and nothing else.
    TestEqual(TEXT("exactly 4 canonical overload declarations emitted"), DeclLines.Num(), 4);

    // Struct definitions must also be unique per namespace.
    for (const auto* StructName : {
        TEXT("struct FBb_BulkA_EntityScript_SpawnParams"),
        TEXT("struct FBb_BulkB_EntityScript_SpawnParams"),
        TEXT("struct FBb_BulkC_EntityScript_SpawnParams"),
        TEXT("struct FBb_BulkD_EntityScript_SpawnParams")})
    {
        TestEqual(FString::Printf(TEXT("%s defined exactly once"), StructName),
            Count_Occurrences(Stub, StructName), 1);
    }

    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Derive_ClassNameFromStructName: inverse of Derive_SpawnParamsStructName.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StubSynthesizer_DeriveClassNameFromStructName,
    "CkAngelscriptGenerator.UnitTests.StubSynthesizer.DeriveClassNameFromStructName",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StubSynthesizer_DeriveClassNameFromStructName::RunTest(const FString&)
{
    TestEqual(TEXT("F<X>_SpawnParams -> U<X>"),
        FCkAsStubSynthesizer::Derive_ClassNameFromStructName(TEXT("FBb_CombatReceiver_DamageReceiver_SpawnParams")),
        FString{TEXT("UBb_CombatReceiver_DamageReceiver")});

    // Round-trip with the forward derivation.
    const auto Forward = FCkAsStubSynthesizer::Derive_SpawnParamsStructName(TEXT("UBb_DayCycle_EntityScript"));
    TestEqual(TEXT("round-trip"),
        FCkAsStubSynthesizer::Derive_ClassNameFromStructName(Forward),
        FString{TEXT("UBb_DayCycle_EntityScript")});

    TestEqual(TEXT("non-F prefix -> empty"),
        FCkAsStubSynthesizer::Derive_ClassNameFromStructName(TEXT("Bb_SpawnParams")), FString{});
    TestEqual(TEXT("no _SpawnParams suffix -> empty"),
        FCkAsStubSynthesizer::Derive_ClassNameFromStructName(TEXT("FBb_SomeRandomStruct")), FString{});
    TestEqual(TEXT("bare F_SpawnParams (empty core) -> empty"),
        FCkAsStubSynthesizer::Derive_ClassNameFromStructName(TEXT("F_SpawnParams")), FString{});
    TestEqual(TEXT("empty -> empty"),
        FCkAsStubSynthesizer::Derive_ClassNameFromStructName(FString{}), FString{});

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Build_EntityScriptParamsStub_FullShape: fielded struct (no defaults) +
// positional ctor + both Params overloads + class-level and per-overload
// dedup markers with canonical (const-stripped) types.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StubSynthesizer_Build_FullShape,
    "CkAngelscriptGenerator.UnitTests.StubSynthesizer.Build_FullShape",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StubSynthesizer_Build_FullShape::RunTest(const FString&)
{
    auto Shape = FCk_AsClassShape{};
    Shape.Found          = true;
    Shape.ClassName      = TEXT("UBb_Fielded_EntityScript");
    Shape.SourceFilePath = TEXT("D:/Test/BB_Fielded.as");
    Shape.FlattenedProperties = {
        FCk_AsExposedProperty{TEXT("FGameplayTag"),                       TEXT("Phase")},
        FCk_AsExposedProperty{TEXT("const UCk_InventoryItem_Definition"), TEXT("Definition")},
        FCk_AsExposedProperty{TEXT("TMap<FGameplayTag, float32>"),        TEXT("Weights")},
    };

    auto Error = FCk_AsParsedError{};
    Error.Kind              = ECk_AsParsedError_Kind::IdentifierNotADataType;
    Error.MissingIdentifier = TEXT("FBb_Fielded_EntityScript_SpawnParams");
    Error.FilePath          = TEXT("D:/Test/Caller.as");
    Error.Line              = 85;
    Error.Column            = 30;

    const auto Stub = FCkAsStubSynthesizer::Build_EntityScriptParamsStub_FullShape(Shape, Error);

    TestTrue(TEXT("not empty"), NOT Stub.IsEmpty());
    TestTrue(TEXT("struct decl"),      Stub.Contains(TEXT("struct FBb_Fielded_EntityScript_SpawnParams")));
    TestTrue(TEXT("field: Phase"),     Stub.Contains(TEXT("    FGameplayTag Phase;")));
    TestTrue(TEXT("field: Definition (verbatim const)"),
        Stub.Contains(TEXT("    const UCk_InventoryItem_Definition Definition;")));
    TestTrue(TEXT("field: Weights (template with comma)"),
        Stub.Contains(TEXT("    TMap<FGameplayTag, float32> Weights;")));
    TestFalse(TEXT("no default initializers on fields"),
        Stub.Contains(TEXT("Phase = FGameplayTag()")));

    TestTrue(TEXT("positional ctor"),
        Stub.Contains(TEXT("FBb_Fielded_EntityScript_SpawnParams(FGameplayTag InPhase, const UCk_InventoryItem_Definition InDefinition, TMap<FGameplayTag, float32> InWeights)")));
    TestTrue(TEXT("ctor assigns Phase"), Stub.Contains(TEXT("        Phase = InPhase;")));

    TestTrue(TEXT("namespace"),        Stub.Contains(TEXT("namespace UBb_Fielded_EntityScript")));
    TestTrue(TEXT("no-arg overload"),  Stub.Contains(TEXT(" Params()")));
    TestTrue(TEXT("all-args overload forwards"),
        Stub.Contains(TEXT("return FBb_Fielded_EntityScript_SpawnParams(InPhase, InDefinition, InWeights);")));

    TestTrue(TEXT("full-shape marker"),
        Stub.Contains(FCkAsStubSynthesizer::Get_FullShapeMarkerLine(TEXT("UBb_Fielded_EntityScript"))));
    TestTrue(TEXT("no-arg end-marker (error-text dedup prefix)"),
        Stub.Contains(TEXT("// End synthesized stub for UBb_Fielded_EntityScript::Params()")));
    TestTrue(TEXT("all-args end-marker uses canonical const-stripped types"),
        Stub.Contains(TEXT("// End synthesized stub for UBb_Fielded_EntityScript::Params(FGameplayTag, UCk_InventoryItem_Definition, TMap<FGameplayTag, float32>)")));

    TestTrue(TEXT("forensic: source path"), Stub.Contains(TEXT("// Source-derived from: D:/Test/BB_Fielded.as")));
    TestTrue(TEXT("forensic: target identifier"), Stub.Contains(TEXT("missing type 'FBb_Fielded_EntityScript_SpawnParams'")));

    // Zero-property shape: no positional ctor, single Params(), single end-marker.
    auto EmptyShape = FCk_AsClassShape{};
    EmptyShape.Found     = true;
    EmptyShape.ClassName = TEXT("UBb_Empty_EntityScript");
    const auto EmptyStub = FCkAsStubSynthesizer::Build_EntityScriptParamsStub_FullShape(EmptyShape, Error);
    TestTrue(TEXT("zero-prop: struct emitted"), EmptyStub.Contains(TEXT("struct FBb_Empty_EntityScript_SpawnParams")));
    TestEqual(TEXT("zero-prop: only the no-arg overload is declared"),
        Count_Occurrences(EmptyStub, TEXT(" Params(")), 1);
    // The ctor decl would sit at 4-space indent; the no-arg overload's
    // `return FBb_...();` body line does not match this needle.
    TestFalse(TEXT("zero-prop: no positional ctor"),
        EmptyStub.Contains(TEXT("    FBb_Empty_EntityScript_SpawnParams(")));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Inject_EntityScriptParamsStub_SourceDerived: end-to-end against a temp
// project tree — the CombatReceiver-shaped wholesale-missing case. The class
// declares 6 ExposeOnSpawn props; the sibling stub must carry all of them
// (so `P.Phase = ...` field-access callers compile), re-fires must no-op,
// and a subsequent ERROR-TEXT inject for the same class's Params() must
// dedup against the full-shape block (cross-path dedup).
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StubSynthesizer_Inject_SourceDerived_EndToEnd,
    "CkAngelscriptGenerator.UnitTests.StubSynthesizer.Inject_SourceDerived_EndToEnd",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StubSynthesizer_Inject_SourceDerived_EndToEnd::RunTest(const FString&)
{
    const auto TempRoot = FPaths::ConvertRelativePathToFull(
        FPaths::ProjectIntermediateDir() / TEXT("CkStubSynthTest_SourceDerived"));
    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);

    const auto ScriptRoot   = TempRoot / TEXT("Script");
    const auto ClassFile    = ScriptRoot / TEXT("Classes/TestSynth_DamageReceiver.as");
    const auto ExpectedStub = ScriptRoot / TEXT("Generated/_StubRecovery_FakeProj_EntitySpawnParams.as");

    IFileManager::Get().MakeDirectory(*FPaths::GetPath(ClassFile), /*Tree=*/true);
    FFileHelper::SaveStringToFile(FString{TEXT("{}")}, *(TempRoot / TEXT("FakeProj.uproject")));
    FFileHelper::SaveStringToFile(FString{TEXT(
        "class UTestSynth_DamageReceiver : UCk_GenericEntityScript_UE\n"
        "{\n"
        "    UPROPERTY(ExposeOnSpawn)\n"
        "    FGameplayTagContainer DataBundleNames;\n"
        "    UPROPERTY(ExposeOnSpawn)\n"
        "    FGameplayTag Phase;\n"
        "    UPROPERTY(ExposeOnSpawn)\n"
        "    FCk_Handle_ResolverTarget ResolverTarget;\n"
        "    UPROPERTY(ExposeOnSpawn)\n"
        "    FCk_Handle_CombatReceiver Receiver;\n"
        "    UPROPERTY(ExposeOnSpawn)\n"
        "    FGameplayTagRequirements GameplayTagRequirements;\n"
        "    UPROPERTY(ExposeOnSpawn)\n"
        "    FGameplayTag HitCue;\n"
        "}\n")}, *ClassFile, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    auto Error = FCk_AsParsedError{};
    Error.Kind              = ECk_AsParsedError_Kind::BareCtorNoMatchingSignatures;
    Error.MissingIdentifier = TEXT("FTestSynth_DamageReceiver_SpawnParams");
    Error.FilePath          = TEXT("D:/Test/Caller.as");
    Error.Line              = 85;
    Error.Column            = 30;

    const auto ResultA = FCkAsStubSynthesizer::Inject_EntityScriptParamsStub_SourceDerived(
        TEXT("UTestSynth_DamageReceiver"), Error, /*InCandidateFilePaths=*/{}, /*InScanRoots=*/{ScriptRoot});

    TestTrue(TEXT("first inject succeeded"), ResultA.Success);
    if (NOT ResultA.Success)
    {
        AddError(FString::Printf(TEXT("Inject failed: %s"), *ResultA.ErrorMessage));
        IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);
        return false;
    }
    TestEqual(TEXT("anchored to the class file's project bucket"), ResultA.TargetFilePath, ExpectedStub);

    auto Stub = FString{};
    TestTrue(TEXT("sibling readable"), FFileHelper::LoadFileToString(Stub, *ExpectedStub));

    // The fields that make `ReceiverParams.Phase = ...`-style callers compile.
    TestTrue(TEXT("field: DataBundleNames"), Stub.Contains(TEXT("    FGameplayTagContainer DataBundleNames;")));
    TestTrue(TEXT("field: Phase typed FGameplayTag"), Stub.Contains(TEXT("    FGameplayTag Phase;")));
    TestTrue(TEXT("field: Receiver"), Stub.Contains(TEXT("    FCk_Handle_CombatReceiver Receiver;")));
    TestTrue(TEXT("field: HitCue"), Stub.Contains(TEXT("    FGameplayTag HitCue;")));
    TestTrue(TEXT("positional ctor assigns"), Stub.Contains(TEXT("        Phase = InPhase;")));
    TestTrue(TEXT("namespace block"), Stub.Contains(TEXT("namespace UTestSynth_DamageReceiver")));

    // Re-fire: no-op success, struct still defined exactly once.
    const auto ResultB = FCkAsStubSynthesizer::Inject_EntityScriptParamsStub_SourceDerived(
        TEXT("UTestSynth_DamageReceiver"), Error, {}, {ScriptRoot});
    TestTrue(TEXT("re-fire reports success (no-op)"), ResultB.Success);

    // Cross-path dedup: an error-text inject for the same class's Params()
    // must no-op against the full-shape block's end-marker. No candidates
    // (the canonical doesn't exist on a fresh clone) — the caller-path
    // anchor resolves the same project bucket and thus the same sibling.
    const auto ErrorText = Make_ParsedError(TEXT("UTestSynth_DamageReceiver"), TEXT("Params"), TEXT(""), *ClassFile);
    const auto ResultC = FCkAsStubSynthesizer::Inject_EntityScriptParamsStub(ErrorText, /*InCandidateFilePaths=*/{});
    TestTrue(TEXT("error-text inject after full shape reports success (no-op)"), ResultC.Success);

    FFileHelper::LoadFileToString(Stub, *ExpectedStub);
    TestEqual(TEXT("struct defined exactly once after all injects"),
        Count_Occurrences(Stub, TEXT("struct FTestSynth_DamageReceiver_SpawnParams")), 1);
    TestEqual(TEXT("no-arg Params() declared exactly once after all injects"),
        Count_Occurrences(Stub, TEXT(" Params()")), 1);

    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Inject_EntityScriptParamsStub_SourceDerived: when the struct already exists
// in the canonical, full-shape synthesis must DEFER (Success = false) — that
// is incremental drift, owned by the battle-tested per-signature error-text
// path; a second struct definition would itself wedge the compile.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StubSynthesizer_Inject_SourceDerived_DefersOnExistingStruct,
    "CkAngelscriptGenerator.UnitTests.StubSynthesizer.Inject_SourceDerived_DefersOnExistingStruct",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StubSynthesizer_Inject_SourceDerived_DefersOnExistingStruct::RunTest(const FString&)
{
    const auto TempRoot = FPaths::ConvertRelativePathToFull(
        FPaths::ProjectIntermediateDir() / TEXT("CkStubSynthTest_SourceDerivedDefer"));
    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);

    const auto ScriptRoot    = TempRoot / TEXT("Script");
    const auto ClassFile     = ScriptRoot / TEXT("Classes/TestSynth_Drift.as");
    const auto CanonicalFile = ScriptRoot / TEXT("Generated/FakeProj_EntitySpawnParams.as");

    IFileManager::Get().MakeDirectory(*FPaths::GetPath(ClassFile), /*Tree=*/true);
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(CanonicalFile), /*Tree=*/true);
    FFileHelper::SaveStringToFile(FString{TEXT("{}")}, *(TempRoot / TEXT("FakeProj.uproject")));
    FFileHelper::SaveStringToFile(FString{TEXT(
        "class UTestSynth_Drift : UCk_GenericEntityScript_UE\n"
        "{\n"
        "    UPROPERTY(ExposeOnSpawn)\n"
        "    int32 Knob;\n"
        "}\n")}, *ClassFile, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    // Canonical already defines the struct (stale shape — incremental drift).
    FFileHelper::SaveStringToFile(FString{TEXT(
        "USTRUCT()\n"
        "struct FTestSynth_Drift_SpawnParams\n"
        "{\n"
        "}\n"
        "namespace UTestSynth_Drift { FTestSynth_Drift_SpawnParams Params() { return FTestSynth_Drift_SpawnParams(); } }\n")},
        *CanonicalFile, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    auto Error = FCk_AsParsedError{};
    Error.Kind             = ECk_AsParsedError_Kind::NoMatchingSignatures;
    Error.TargetNamespace  = TEXT("UTestSynth_Drift");
    Error.FunctionName     = TEXT("Params");
    Error.ArgsList         = TEXT("int32");
    Error.FilePath         = TEXT("D:/Test/Caller.as");

    const auto Result = FCkAsStubSynthesizer::Inject_EntityScriptParamsStub_SourceDerived(
        TEXT("UTestSynth_Drift"), Error, /*InCandidateFilePaths=*/{CanonicalFile}, /*InScanRoots=*/{ScriptRoot});

    TestFalse(TEXT("defers (Success = false)"), Result.Success);
    TestTrue(TEXT("reason mentions deferral to per-signature path"),
        Result.ErrorMessage.Contains(TEXT("defers")));
    TestFalse(TEXT("no sibling stub written"),
        IFileManager::Get().FileExists(*(ScriptRoot / TEXT("Generated/_StubRecovery_FakeProj_EntitySpawnParams.as"))));

    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Build_EntityScriptParamsStub: a bare-nullptr call-site arg arrives as the
// compiler placeholder `<null handle>` — it must be emitted as a UObject
// fallback param, never verbatim. Verbatim emission makes the stub file
// unparseable ("Expected data type / Instead found '<'"), which wedges every
// subsequent compile and is unrecoverable because the corrupt file is
// self-heal's own output (2026-06-10 UBb_StoreDriver_EntityScript incident).
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StubSynthesizer_Build_NullHandleArg_EmitsUObjectFallback,
    "CkAngelscriptGenerator.UnitTests.StubSynthesizer.Build_NullHandleArg_EmitsUObjectFallback",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StubSynthesizer_Build_NullHandleArg_EmitsUObjectFallback::RunTest(const FString&)
{
    const auto Error = Make_ParsedError(
        TEXT("UBb_StoreDriver_EntityScript"),
        TEXT("Params"),
        TEXT("FTransform, bool, <null handle>"));

    const auto Stub = FCkAsStubSynthesizer::Build_EntityScriptParamsStub(Error, /*InEmitStruct=*/false);

    TestTrue(TEXT("placeholder emitted as UObject fallback param"),
        Stub.Contains(TEXT("Params(FTransform Arg0, bool Arg1, UObject Arg2)")));
    TestFalse(TEXT("placeholder never emitted as a param type"),
        Stub.Contains(TEXT("<null handle> Arg")));
    // The diagnostic `// Target:` line keeps the raw error verbatim for
    // forensics — a comment can hold the placeholder safely.
    TestTrue(TEXT("Target comment preserves the raw error"),
        Stub.Contains(TEXT("// Target: UBb_StoreDriver_EntityScript::Params(FTransform, bool, <null handle>)")));
    // The end-marker uses the normalized list, so the placeholder never leaks
    // into the dedup key either.
    TestTrue(TEXT("end-marker uses the normalized (UObject) list"),
        Stub.Contains(TEXT("// End synthesized stub for UBb_StoreDriver_EntityScript::Params(FTransform, bool, UObject)")));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Same-arity ambiguity gate, typed-first order: once a fully-typed stub
// exists, a nullptr-variant (UObject fallback) of the same arity must NOT be
// appended — nullptr binds against the typed stub, and the pair would be
// mutually ambiguous ("Multiple matching signatures", which the dispatcher
// cannot recognize or heal). A fallback variant of a DIFFERENT arity is a
// genuinely distinct overload and must still land.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StubSynthesizer_Inject_NullVariant_SkippedWhenTypedSameArityExists,
    "CkAngelscriptGenerator.UnitTests.StubSynthesizer.Inject_NullVariant_SkippedWhenTypedSameArityExists",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StubSynthesizer_Inject_NullVariant_SkippedWhenTypedSameArityExists::RunTest(const FString&)
{
    const auto TempRoot         = FPaths::ProjectIntermediateDir() / TEXT("CkStubSynthTest_NullVsTyped");
    const auto FixtureFile      = TempRoot / TEXT("BusterBlock_EntitySpawnParams.as");
    const auto ExpectedStubFile = TempRoot / TEXT("_StubRecovery_BusterBlock_EntitySpawnParams.as");
    IFileManager::Get().MakeDirectory(*TempRoot, /*Tree=*/true);

    const auto Original = FString{TEXT("// References UBb_StoreDriver_EntityScript as a string.\n")};
    FFileHelper::SaveStringToFile(Original, *FixtureFile,
        FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    // Call site A passes a real config object; call site B passes nullptr.
    const auto ErrorTyped = Make_ParsedError(
        TEXT("UBb_StoreDriver_EntityScript"), TEXT("Params"),
        TEXT("FTransform, UBb_StoreCustomization_Config"),
        TEXT("D:/Test/CallerA.as"), 100, 9);
    const auto ErrorNull = Make_ParsedError(
        TEXT("UBb_StoreDriver_EntityScript"), TEXT("Params"),
        TEXT("FTransform, <null handle>"),
        TEXT("D:/Test/CallerB.as"), 200, 13);

    const auto ResultTyped = FCkAsStubSynthesizer::Inject_EntityScriptParamsStub(ErrorTyped, {FixtureFile});
    TestTrue(TEXT("typed inject succeeded"), ResultTyped.Success);

    const auto ResultNull = FCkAsStubSynthesizer::Inject_EntityScriptParamsStub(ErrorNull, {FixtureFile});
    TestTrue(TEXT("null-variant inject reports success (no-op)"), ResultNull.Success);

    auto Stub = FString{};
    TestTrue(TEXT("sibling readable"), FFileHelper::LoadFileToString(Stub, *ExpectedStubFile));

    auto Cursor   = 0;
    auto HitCount = 0;
    const auto Needle = FString{TEXT(" Params(")};
    while ((Cursor = Stub.Find(Needle, ESearchCase::CaseSensitive, ESearchDir::FromStart, Cursor)) != INDEX_NONE)
    {
        ++HitCount;
        Cursor += Needle.Len();
    }
    TestEqual(TEXT("exactly one Params(...) declaration"), HitCount, 1);
    TestTrue(TEXT("the typed stub is the one that landed"),
        Stub.Contains(TEXT("Params(FTransform Arg0, UBb_StoreCustomization_Config Arg1)")));
    TestFalse(TEXT("no UObject-fallback overload landed"),
        Stub.Contains(TEXT("UObject Arg1)")));

    // A fallback variant of a DIFFERENT arity is a distinct overload — it
    // must still append.
    const auto ErrorNull3Arg = Make_ParsedError(
        TEXT("UBb_StoreDriver_EntityScript"), TEXT("Params"),
        TEXT("FTransform, bool, <null handle>"),
        TEXT("D:/Test/CallerC.as"), 300, 5);
    const auto ResultNull3Arg = FCkAsStubSynthesizer::Inject_EntityScriptParamsStub(ErrorNull3Arg, {FixtureFile});
    TestTrue(TEXT("different-arity null variant succeeded"), ResultNull3Arg.Success);

    TestTrue(TEXT("sibling re-readable"), FFileHelper::LoadFileToString(Stub, *ExpectedStubFile));
    TestTrue(TEXT("different-arity fallback overload landed"),
        Stub.Contains(TEXT("Params(FTransform Arg0, bool Arg1, UObject Arg2)")));

    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Same-arity ambiguity gate, fallback-first order: when the nullptr variant
// landed first (UObject fallback stub), a later fully-typed variant of the
// same arity must NOT be appended — the typed call site's handle implicitly
// converts to UObject, so the existing fallback stub already satisfies it,
// and appending the typed twin would create the same mutual ambiguity.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StubSynthesizer_Inject_TypedVariant_SkippedWhenFallbackSameArityExists,
    "CkAngelscriptGenerator.UnitTests.StubSynthesizer.Inject_TypedVariant_SkippedWhenFallbackSameArityExists",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StubSynthesizer_Inject_TypedVariant_SkippedWhenFallbackSameArityExists::RunTest(const FString&)
{
    const auto TempRoot         = FPaths::ProjectIntermediateDir() / TEXT("CkStubSynthTest_TypedVsNull");
    const auto FixtureFile      = TempRoot / TEXT("BusterBlock_EntitySpawnParams.as");
    const auto ExpectedStubFile = TempRoot / TEXT("_StubRecovery_BusterBlock_EntitySpawnParams.as");
    IFileManager::Get().MakeDirectory(*TempRoot, /*Tree=*/true);

    const auto Original = FString{TEXT("// References UBb_StoreDriver_EntityScript as a string.\n")};
    FFileHelper::SaveStringToFile(Original, *FixtureFile,
        FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    const auto ErrorNull = Make_ParsedError(
        TEXT("UBb_StoreDriver_EntityScript"), TEXT("Params"),
        TEXT("FTransform, <null handle>"),
        TEXT("D:/Test/CallerB.as"), 200, 13);
    const auto ErrorTyped = Make_ParsedError(
        TEXT("UBb_StoreDriver_EntityScript"), TEXT("Params"),
        TEXT("FTransform, UBb_StoreCustomization_Config"),
        TEXT("D:/Test/CallerA.as"), 100, 9);

    const auto ResultNull = FCkAsStubSynthesizer::Inject_EntityScriptParamsStub(ErrorNull, {FixtureFile});
    TestTrue(TEXT("null-variant inject succeeded"), ResultNull.Success);

    const auto ResultTyped = FCkAsStubSynthesizer::Inject_EntityScriptParamsStub(ErrorTyped, {FixtureFile});
    TestTrue(TEXT("typed inject reports success (no-op)"), ResultTyped.Success);

    auto Stub = FString{};
    TestTrue(TEXT("sibling readable"), FFileHelper::LoadFileToString(Stub, *ExpectedStubFile));

    auto Cursor   = 0;
    auto HitCount = 0;
    const auto Needle = FString{TEXT(" Params(")};
    while ((Cursor = Stub.Find(Needle, ESearchCase::CaseSensitive, ESearchDir::FromStart, Cursor)) != INDEX_NONE)
    {
        ++HitCount;
        Cursor += Needle.Len();
    }
    TestEqual(TEXT("exactly one Params(...) declaration"), HitCount, 1);
    TestTrue(TEXT("the fallback stub is the one that landed"),
        Stub.Contains(TEXT("Params(FTransform Arg0, UObject Arg1)")));
    TestFalse(TEXT("no typed twin landed"),
        Stub.Contains(TEXT("UBb_StoreCustomization_Config Arg1)")));

    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
