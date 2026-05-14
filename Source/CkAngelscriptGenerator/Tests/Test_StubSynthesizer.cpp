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
// dedup gate in Try_AtomicWriteOrAppend_StubFile_Utf16 scans existing
// sibling content for the unique `// End synthesized stub for <NS>::<FUNC>`
// marker line and short-circuits the append when present.
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

#endif // WITH_DEV_AUTOMATION_TESTS
