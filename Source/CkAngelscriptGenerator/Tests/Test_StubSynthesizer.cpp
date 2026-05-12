// Tests for the AS stub synthesizer (Rev 10 dispatcher recovery strategy #1).
//
// Coverage:
//   * Derive_SpawnParamsStructName — name-shape conversion
//   * Build_EntityScriptParamsStub — pure-string output for the three argument
//     shapes seen in practice (no-arg, single-typed-arg, multi-arg) and the
//     struct-emission toggle
//   * Has_SpawnParamsStruct       — file-contents heuristic
//   * Find_TargetFile_ByContent   — candidate-file resolution + ambiguity guard
//   * Inject_EntityScriptParamsStub — end-to-end with a temp-file fixture,
//     verifying the original content is preserved and the stub is appended

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

    TestTrue(TEXT("Success"),         Result.Success);
    TestEqual(TEXT("Target = fixture"), Result.TargetFilePath, FixtureFile);
    TestFalse(TEXT("InjectedBlock non-empty"), Result.InjectedBlock.IsEmpty());

    // Read back the updated file.
    auto AfterInject = FString{};
    FFileHelper::LoadFileToString(AfterInject, *FixtureFile);

    TestTrue(TEXT("original content preserved"),     AfterInject.StartsWith(Original));
    TestTrue(TEXT("appended stub appears in file"),  AfterInject.Contains(Result.InjectedBlock));
    TestTrue(TEXT("appended namespace is correct"),
        AfterInject.Contains(TEXT("namespace UBb_DeliveryTruck_EntityScript")));
    TestTrue(TEXT("appended Params signature is correct"),
        AfterInject.Contains(TEXT("Params(FTransform Arg0)")));
    TestFalse(TEXT("does NOT re-emit struct (already exists)"),
        Result.InjectedBlock.Contains(TEXT("USTRUCT()")));
    TestTrue(TEXT("marker comment is present"),
        AfterInject.Contains(FCkAsStubSynthesizer::Get_MarkerComment()));

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
    TestTrue(TEXT("injected block includes USTRUCT (struct missing)"),
        Result.InjectedBlock.Contains(TEXT("USTRUCT()")));
    TestTrue(TEXT("injected block includes struct decl"),
        Result.InjectedBlock.Contains(TEXT("struct FBb_Orphan_EntityScript_SpawnParams")));

    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Inject_EntityScriptParamsStub: no candidate file references the target —
// must fail cleanly, NOT touch any file, return a descriptive error.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StubSynthesizer_Inject_NoCandidate,
    "CkAngelscriptGenerator.UnitTests.StubSynthesizer.Inject_NoCandidate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StubSynthesizer_Inject_NoCandidate::RunTest(const FString&)
{
    const auto TempRoot = FPaths::ProjectIntermediateDir() / TEXT("CkStubSynthTest_NoCandidate");
    const auto FixtureFile = TempRoot / TEXT("Unrelated_EntitySpawnParams.as");
    IFileManager::Get().MakeDirectory(*TempRoot, /*Tree=*/true);

    const auto Original = FString{TEXT("// Unrelated content — does not reference our target.\n")};
    FFileHelper::SaveStringToFile(Original, *FixtureFile,
        FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    const auto Error  = Make_ParsedError(TEXT("UBb_AbsentlyDefined_EntityScript"), TEXT("Params"), TEXT(""));
    const auto Result = FCkAsStubSynthesizer::Inject_EntityScriptParamsStub(Error, {FixtureFile});

    TestFalse(TEXT("Success = false"),       Result.Success);
    TestFalse(TEXT("ErrorMessage populated"), Result.ErrorMessage.IsEmpty());

    // File must be untouched.
    auto AfterAttempt = FString{};
    FFileHelper::LoadFileToString(AfterAttempt, *FixtureFile);
    TestEqual(TEXT("file unchanged on failure"), AfterAttempt, Original);

    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
