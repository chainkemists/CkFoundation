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

    // The `" Params("` needle (leading space) is what separates a declaration line from the
    // `::Params(` of comment markers and the `return F<X>_SpawnParams();` of bodies.
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

    return true;
}

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StubSynthesizer_FindTargetFile_ByContent,
    "CkAngelscriptGenerator.UnitTests.StubSynthesizer.FindTargetFile_ByContent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StubSynthesizer_FindTargetFile_ByContent::RunTest(const FString&)
{
    const auto TempRoot   = FPaths::ProjectIntermediateDir() / TEXT("CkStubSynthTest");
    const auto FileA      = TempRoot / TEXT("PluginA_EntitySpawnParams.as");
    const auto FileB      = TempRoot / TEXT("PluginB_EntitySpawnParams.as");
    const auto FileC      = TempRoot / TEXT("PluginC_EntitySpawnParams.as");
    IFileManager::Get().MakeDirectory(*TempRoot, /*Tree=*/true);
    FFileHelper::SaveStringToFile(FString{TEXT("// PluginA — references UBb_Foo_EntityScript here\n")}, *FileA);
    FFileHelper::SaveStringToFile(FString{TEXT("// PluginB — no relevant content\n")},                  *FileB);
    FFileHelper::SaveStringToFile(FString{TEXT("// PluginC — also references UBb_Foo_EntityScript\n")}, *FileC);

    {
        const auto Match = FCkAsStubSynthesizer::Find_TargetFile_ByContent(
            TEXT("UBb_Foo_EntityScript"),
            {FileA, FileB});
        TestEqual(TEXT("single match -> that path"), Match, FileA);
    }

    {
        const auto Match = FCkAsStubSynthesizer::Find_TargetFile_ByContent(
            TEXT("UBb_DoesNotExist_EntityScript"),
            {FileA, FileB, FileC});
        TestEqual(TEXT("no match -> empty"), Match, FString{});
    }

    {
        const auto Match = FCkAsStubSynthesizer::Find_TargetFile_ByContent(
            TEXT("UBb_Foo_EntityScript"),
            {FileA, FileB, FileC});
        TestEqual(TEXT("ambiguous match (PluginA + PluginC) -> empty (defensive)"), Match, FString{});
    }

    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);
    return true;
}

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

    // Corrupted-canonical fixture: struct intact, namespace renamed.
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

    auto CanonicalAfter = FString{};
    FFileHelper::LoadFileToString(CanonicalAfter, *FixtureFile);
    TestEqual(TEXT("canonical fixture untouched"), CanonicalAfter, Original);

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

    auto CanonicalAfter = FString{};
    FFileHelper::LoadFileToString(CanonicalAfter, *FixtureFile);
    TestEqual(TEXT("canonical untouched"), CanonicalAfter, Original);

    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StubSynthesizer_Inject_AnchorsByCallerPath_Project,
    "CkAngelscriptGenerator.UnitTests.StubSynthesizer.Inject_AnchorsByCallerPath_Project",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StubSynthesizer_Inject_AnchorsByCallerPath_Project::RunTest(const FString&)
{
    // Absolute up-front: Anchor_ByCallerAsPath canonicalizes via ConvertRelativePathToFull.
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StubSynthesizer_Inject_AnchorsByCallerPath_NoManifest_Fails,
    "CkAngelscriptGenerator.UnitTests.StubSynthesizer.Inject_AnchorsByCallerPath_NoManifest_Fails",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StubSynthesizer_Inject_AnchorsByCallerPath_NoManifest_Fails::RunTest(const FString&)
{
    // No manifest ancestor anywhere on the chain, and outside the engine + project trees:
    // without a clean failure the synthesizer would write a stub at the filesystem root.
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StubSynthesizer_AnchorByCallerAsPath_Direct,
    "CkAngelscriptGenerator.UnitTests.StubSynthesizer.AnchorByCallerAsPath_Direct",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StubSynthesizer_AnchorByCallerAsPath_Direct::RunTest(const FString&)
{
    const auto TempRoot = FPaths::ConvertRelativePathToFull(
        FPaths::ProjectIntermediateDir() / TEXT("CkStubSynthTest_AnchorDirect"));
    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);

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

    // Depends on the block above having left a .uproject at TempRoot — that is what makes
    // this also the "plugin wins when both ancestors exist" case.
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

    {
        TestEqual(TEXT("empty caller -> empty"),
            FCkAsStubSynthesizer::Anchor_ByCallerAsPath(FString{}), FString{});
    }

    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);
    return true;
}

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

    auto Cursor   = 0;
    auto HitCount = 0;
    const auto Needle = FString{TEXT("AUTO-GENERATED RECOVERY STUBS")};
    while ((Cursor = Stub.Find(Needle, ESearchCase::IgnoreCase, ESearchDir::FromStart, Cursor)) != INDEX_NONE)
    {
        ++HitCount;
        Cursor += Needle.Len();
    }
    TestEqual(TEXT("header banner appears exactly once"), HitCount, 1);

    auto CanonicalAfter = FString{};
    FFileHelper::LoadFileToString(CanonicalAfter, *FixtureFile);
    TestEqual(TEXT("canonical untouched after both injects"), CanonicalAfter, Original);

    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);
    return true;
}

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

    auto Cursor   = 0;
    auto HitCount = 0;
    const auto Needle = FString{TEXT("// End synthesized stub for UBb_Gamma_EntityScript::Params")};
    while ((Cursor = Stub.Find(Needle, ESearchCase::CaseSensitive, ESearchDir::FromStart, Cursor)) != INDEX_NONE)
    {
        ++HitCount;
        Cursor += Needle.Len();
    }
    TestEqual(TEXT("end-marker for the accessor appears exactly once"), HitCount, 1);

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
// The canonicalization shared by stub emission, the dedup end-marker, and the dispatcher's
// convergence key — all three must agree or the dedup gates stop matching.
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

    auto Cursor   = 0;
    auto HitCount = 0;
    const auto Needle = FString{TEXT(" Params(")};
    while ((Cursor = Stub.Find(Needle, ESearchCase::CaseSensitive, ESearchDir::FromStart, Cursor)) != INDEX_NONE)
    {
        ++HitCount;
        Cursor += Needle.Len();
    }
    TestEqual(TEXT("Params(...) declaration appears exactly once"), HitCount, 1);

    TestTrue(TEXT("emitted param list is value-typed (accepts literal and lvalue)"),
        Stub.Contains(TEXT("Params(FTransform Arg0, FCk_Handle_CheckoutCounter Arg1, EBb_Role Arg2, int Arg3, FVector Arg4)")));
    TestFalse(TEXT("no reference-typed int param emitted"), Stub.Contains(TEXT("int& Arg3")));

    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);
    return true;
}


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

    const auto Drain1 = TArray<FCk_AsParsedError>{
        Make_ParsedError(TEXT("UBb_BulkA_EntityScript"), TEXT("Params"), TEXT("")),
        Make_ParsedError(TEXT("UBb_BulkB_EntityScript"), TEXT("Params"), TEXT("const FTransform")),
        Make_ParsedError(TEXT("UBb_BulkB_EntityScript"), TEXT("Params"), TEXT("FTransform")),
        Make_ParsedError(TEXT("UBb_BulkC_EntityScript"), TEXT("Params"), TEXT("FTransform, const int32")),
        Make_ParsedError(TEXT("UBb_BulkD_EntityScript"), TEXT("Params"), TEXT("const UClass, bool")),
    };

    // The same batch re-fires (compile still failing), const-qualifications flipped.
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

    constexpr auto DistinctCanonicalOverloads = 4;
    TestEqual(TEXT("exactly 4 canonical overload declarations emitted"),
        DeclLines.Num(), DistinctCanonicalOverloads);

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StubSynthesizer_DeriveClassNameFromStructName,
    "CkAngelscriptGenerator.UnitTests.StubSynthesizer.DeriveClassNameFromStructName",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StubSynthesizer_DeriveClassNameFromStructName::RunTest(const FString&)
{
    TestEqual(TEXT("F<X>_SpawnParams -> U<X>"),
        FCkAsStubSynthesizer::Derive_ClassNameFromStructName(TEXT("FBb_CombatReceiver_DamageReceiver_SpawnParams")),
        FString{TEXT("UBb_CombatReceiver_DamageReceiver")});

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

    auto EmptyShape = FCk_AsClassShape{};
    EmptyShape.Found     = true;
    EmptyShape.ClassName = TEXT("UBb_Empty_EntityScript");
    const auto EmptyStub = FCkAsStubSynthesizer::Build_EntityScriptParamsStub_FullShape(EmptyShape, Error);
    TestTrue(TEXT("zero-prop: struct emitted"), EmptyStub.Contains(TEXT("struct FBb_Empty_EntityScript_SpawnParams")));
    TestEqual(TEXT("zero-prop: only the no-arg overload is declared"),
        Count_Occurrences(EmptyStub, TEXT(" Params(")), 1);
    // The 4-space indent is what excludes the no-arg overload's `return FBb_...();` body line.
    TestFalse(TEXT("zero-prop: no positional ctor"),
        EmptyStub.Contains(TEXT("    FBb_Empty_EntityScript_SpawnParams(")));

    return true;
}

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

    TestTrue(TEXT("field: DataBundleNames"), Stub.Contains(TEXT("    FGameplayTagContainer DataBundleNames;")));
    TestTrue(TEXT("field: Phase typed FGameplayTag"), Stub.Contains(TEXT("    FGameplayTag Phase;")));
    TestTrue(TEXT("field: Receiver"), Stub.Contains(TEXT("    FCk_Handle_CombatReceiver Receiver;")));
    TestTrue(TEXT("field: HitCue"), Stub.Contains(TEXT("    FGameplayTag HitCue;")));
    TestTrue(TEXT("positional ctor assigns"), Stub.Contains(TEXT("        Phase = InPhase;")));
    TestTrue(TEXT("namespace block"), Stub.Contains(TEXT("namespace UTestSynth_DamageReceiver")));

    const auto ResultB = FCkAsStubSynthesizer::Inject_EntityScriptParamsStub_SourceDerived(
        TEXT("UTestSynth_DamageReceiver"), Error, {}, {ScriptRoot});
    TestTrue(TEXT("struct-shaped re-fire reports success (no-op)"), ResultB.Success);

    // A signature miss in the SAME session that wrote the full shape has never been
    // compile-tested against it, so it must no-op and let the next compile decide —
    // deferring here appended junk overloads for calls the full shape already satisfies.
    auto DivergentSignature = FCk_AsParsedError{};
    DivergentSignature.Kind            = ECk_AsParsedError_Kind::NoMatchingSignatures;
    DivergentSignature.TargetNamespace = TEXT("UTestSynth_DamageReceiver");
    DivergentSignature.FunctionName    = TEXT("Params");
    DivergentSignature.ArgsList        = TEXT("FGameplayTag, const int");
    DivergentSignature.FilePath        = *ClassFile;
    const auto ResultSameSession = FCkAsStubSynthesizer::Inject_EntityScriptParamsStub_SourceDerived(
        TEXT("UTestSynth_DamageReceiver"), DivergentSignature, {}, {ScriptRoot});
    TestTrue(TEXT("same-session signature re-fire no-ops (Success = true)"), ResultSameSession.Success);

    // The NEXT process is the opposite case: the on-disk full shape was already part of the
    // compile that just failed, so the caller genuinely diverges and the inject must DEFER —
    // that is what lets the dispatcher's error-text fallback append the per-signature overload.
    FCkAsStubSynthesizer::Reset_SessionState_ForTests();
    const auto ResultNextSession = FCkAsStubSynthesizer::Inject_EntityScriptParamsStub_SourceDerived(
        TEXT("UTestSynth_DamageReceiver"), DivergentSignature, {}, {ScriptRoot});
    TestFalse(TEXT("next-session signature miss against on-disk full shape DEFERS (Success = false)"), ResultNextSession.Success);
    TestTrue(TEXT("deferral reason mentions per-signature path"),
        ResultNextSession.ErrorMessage.Contains(TEXT("per-signature")));

    // Cross-path dedup, with no candidates because a fresh clone has no canonical: the
    // caller-path anchor resolves the same project bucket and thus the same sibling.
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
// A second struct definition would itself wedge the compile, so the synthesizer never mutates
// the canonical — it fails with StructExistsInCanonical and the dispatcher escalates.
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

    TestFalse(TEXT("fails (Success = false)"), Result.Success);
    TestTrue(TEXT("FailReason is StructExistsInCanonical (quarantine-escalation trigger)"),
        Result.FailReason == ECk_StubInjectFailReason::StructExistsInCanonical);
    TestEqual(TEXT("canonical path surfaced for the dispatcher's escalation"),
        Result.CanonicalFilePath, CanonicalFile);
    TestFalse(TEXT("no sibling stub written"),
        IFileManager::Get().FileExists(*(ScriptRoot / TEXT("Generated/_StubRecovery_FakeProj_EntitySpawnParams.as"))));
    TestTrue(TEXT("canonical untouched by the synthesizer itself"),
        IFileManager::Get().FileExists(*CanonicalFile));

    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Emitting the `<null handle>` placeholder verbatim makes the stub file unparseable, which is
// unrecoverable — the corrupt file is self-heal's own output, so it cannot heal itself.
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
    // A comment can hold the placeholder safely, so `// Target:` keeps the raw error verbatim.
    TestTrue(TEXT("Target comment preserves the raw error"),
        Stub.Contains(TEXT("// Target: UBb_StoreDriver_EntityScript::Params(FTransform, bool, <null handle>)")));
    TestTrue(TEXT("end-marker uses the normalized (UObject) list"),
        Stub.Contains(TEXT("// End synthesized stub for UBb_StoreDriver_EntityScript::Params(FTransform, bool, UObject)")));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// nullptr binds against a typed same-arity stub, so the pair would be mutually ambiguous
// ("Multiple matching signatures" — an error kind the dispatcher cannot recognize or heal).
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

    // The gate fails loudly rather than faking success: the existing typed stub does not
    // necessarily satisfy the nullptr caller's other args, and the dispatcher needs the
    // SameArityAmbiguous reason to escalate to canonical quarantine.
    const auto ResultNull = FCkAsStubSynthesizer::Inject_EntityScriptParamsStub(ErrorNull, {FixtureFile});
    TestFalse(TEXT("null-variant inject FAILS (no fake success)"), ResultNull.Success);
    TestTrue(TEXT("FailReason is SameArityAmbiguous"),
        ResultNull.FailReason == ECk_StubInjectFailReason::SameArityAmbiguous);

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

    const auto ErrorNull3Arg = Make_ParsedError(
        TEXT("UBb_StoreDriver_EntityScript"), TEXT("Params"),
        TEXT("FTransform, bool, <null handle>"),
        TEXT("D:/Test/CallerC.as"), 300, 5);
    const auto ResultNull3Arg = FCkAsStubSynthesizer::Inject_EntityScriptParamsStub(ErrorNull3Arg, {FixtureFile});
    TestTrue(TEXT("different-arity null variant is a distinct overload and still appends"),
        ResultNull3Arg.Success);

    TestTrue(TEXT("sibling re-readable"), FFileHelper::LoadFileToString(Stub, *ExpectedStubFile));
    TestTrue(TEXT("different-arity fallback overload landed"),
        Stub.Contains(TEXT("Params(FTransform Arg0, bool Arg1, UObject Arg2)")));

    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// The reverse order: a typed call site's handle implicitly converts to UObject, so an existing
// same-arity fallback stub already satisfies it and the typed twin would re-create the ambiguity.
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
    TestFalse(TEXT("typed inject FAILS (no fake success)"), ResultTyped.Success);
    TestTrue(TEXT("FailReason is SameArityAmbiguous"),
        ResultTyped.FailReason == ECk_StubInjectFailReason::SameArityAmbiguous);

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

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StubSynthesizer_EnumerateEntityScriptNamespaces,
    "CkAngelscriptGenerator.UnitTests.StubSynthesizer.EnumerateEntityScriptNamespaces",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StubSynthesizer_EnumerateEntityScriptNamespaces::RunTest(const FString&)
{
    const auto Contents = FString{TEXT(
        "// header noise\n"
        "USTRUCT()\n"
        "struct FTestEnum_A_SpawnParams\n"
        "{\n"
        "}\n"
        "namespace UTestEnum_A\n"
        "{\n"
        "    FTestEnum_A_SpawnParams Params() { return FTestEnum_A_SpawnParams(); }\n"
        "}\n"
        "struct FTestEnum_B_SpawnParams\n"
        "{\n"
        "}\n"
        "namespace UTestEnum_B {\n"
        "}\n"
        "namespace UTestEnum_NoStruct\n"   // no F..._SpawnParams struct in text — ignored
        "{\n"
        "}\n"
        "namespace assets\n"               // not U-prefixed — ignored
        "{\n"
        "}\n"
        "namespace UTestEnum_A\n"          // duplicate — deduped
        "{\n"
        "}\n")};

    const auto Names = FCkAsStubSynthesizer::Enumerate_EntityScriptNamespaces(Contents);

    TestEqual(TEXT("two namespaces enumerated"), Names.Num(), 2);
    if (Names.Num() == 2)
    {
        TestEqual(TEXT("first is UTestEnum_A"),  Names[0], FString{TEXT("UTestEnum_A")});
        TestEqual(TEXT("second is UTestEnum_B (same-line brace tolerated)"), Names[1], FString{TEXT("UTestEnum_B")});
    }

    TestEqual(TEXT("empty contents -> empty"),
        FCkAsStubSynthesizer::Enumerate_EntityScriptNamespaces(FString{}).Num(), 0);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StubSynthesizer_Quarantine_StaleCanonical_EndToEnd,
    "CkAngelscriptGenerator.UnitTests.StubSynthesizer.Quarantine_StaleCanonical_EndToEnd",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StubSynthesizer_Quarantine_StaleCanonical_EndToEnd::RunTest(const FString&)
{
    FCkAsStubSynthesizer::Reset_SessionState_ForTests();

    const auto TempRoot = FPaths::ConvertRelativePathToFull(
        FPaths::ProjectIntermediateDir() / TEXT("CkStubSynthTest_Quarantine"));
    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);

    const auto ScriptRoot    = TempRoot / TEXT("Script");
    const auto ClassFile     = ScriptRoot / TEXT("Classes/TestQ_Driver.as");
    const auto CanonicalFile = ScriptRoot / TEXT("Generated/FakeProj_EntitySpawnParams.as");
    const auto SiblingFile   = ScriptRoot / TEXT("Generated/_StubRecovery_FakeProj_EntitySpawnParams.as");

    IFileManager::Get().MakeDirectory(*FPaths::GetPath(ClassFile),     /*Tree=*/true);
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(CanonicalFile), /*Tree=*/true);
    FFileHelper::SaveStringToFile(FString{TEXT("{}")}, *(TempRoot / TEXT("FakeProj.uproject")));

    FFileHelper::SaveStringToFile(FString{TEXT(
        "class UTestQ_Driver : UCk_GenericEntityScript_UE\n"
        "{\n"
        "    UPROPERTY(ExposeOnSpawn)\n"
        "    FGameplayTag Phase;\n"
        "    UPROPERTY(ExposeOnSpawn)\n"
        "    FCk_Handle StoreEntity;\n"
        "    UPROPERTY(ExposeOnSpawn)\n"
        "    int32 Knob;\n"
        "}\n")}, *ClassFile, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    // Stale canonical: a TWO-param accessor from before the third prop landed.
    FFileHelper::SaveStringToFile(FString{TEXT(
        "USTRUCT()\n"
        "struct FTestQ_Driver_SpawnParams\n"
        "{\n"
        "    UPROPERTY()\n"
        "    FGameplayTag Phase;\n"
        "    UPROPERTY()\n"
        "    FCk_Handle StoreEntity;\n"
        "}\n"
        "namespace UTestQ_Driver\n"
        "{\n"
        "    FTestQ_Driver_SpawnParams Params() { return FTestQ_Driver_SpawnParams(); }\n"
        "    FTestQ_Driver_SpawnParams Params(FGameplayTag InPhase, FCk_Handle InStoreEntity) { return FTestQ_Driver_SpawnParams(); }\n"
        "}\n")}, *CanonicalFile, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    const auto Error = Make_ParsedError(
        TEXT("UTestQ_Driver"), TEXT("Params"), TEXT("FGameplayTag, FCk_Handle, const int"));

    const auto Result = FCkAsStubSynthesizer::Quarantine_And_ResynthesizeFullShapes(
        CanonicalFile, {TEXT("UTestQ_Driver")}, Error, {ScriptRoot});

    TestTrue(TEXT("quarantine succeeded"), Result.Success);
    if (NOT Result.Success)
    { AddError(FString::Printf(TEXT("Quarantine failed: %s"), *Result.ErrorMessage)); }

    TestFalse(TEXT("stale canonical deleted"), IFileManager::Get().FileExists(*CanonicalFile));

    auto ForensicMatches = TArray<FString>{};
    IFileManager::Get().FindFiles(ForensicMatches,
        *(FPaths::ProjectSavedDir() / TEXT("CkSelfHeal/Quarantine") / TEXT("FakeProj_EntitySpawnParams.as.stale_*")),
        /*Files=*/true, /*Dirs=*/false);
    TestTrue(TEXT("forensic copy exists under Saved/CkSelfHeal/Quarantine"), ForensicMatches.Num() >= 1);

    auto Stub = FString{};
    TestTrue(TEXT("sibling rebuilt"), FFileHelper::LoadFileToString(Stub, *SiblingFile));
    TestEqual(TEXT("struct defined exactly once"),
        Count_Occurrences(Stub, TEXT("struct FTestQ_Driver_SpawnParams")), 1);
    TestTrue(TEXT("full shape carries the NEW third prop"),
        Stub.Contains(TEXT("    int32 Knob;")));
    TestTrue(TEXT("exact-typed positional ctor (all three props)"),
        Stub.Contains(TEXT("FTestQ_Driver_SpawnParams(FGameplayTag InPhase, FCk_Handle InStoreEntity, int32 InKnob)")));
    TestTrue(TEXT("full-shape marker present"),
        Stub.Contains(FCkAsStubSynthesizer::Get_FullShapeMarkerLine(TEXT("UTestQ_Driver"))));

    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StubSynthesizer_Quarantine_RebuildsSibling_DroppingWedgedErrorTextStub,
    "CkAngelscriptGenerator.UnitTests.StubSynthesizer.Quarantine_RebuildsSibling_DroppingWedgedErrorTextStub",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StubSynthesizer_Quarantine_RebuildsSibling_DroppingWedgedErrorTextStub::RunTest(const FString&)
{
    FCkAsStubSynthesizer::Reset_SessionState_ForTests();

    const auto TempRoot = FPaths::ConvertRelativePathToFull(
        FPaths::ProjectIntermediateDir() / TEXT("CkStubSynthTest_QuarantineWedge"));
    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);

    const auto ScriptRoot    = TempRoot / TEXT("Script");
    const auto ClassFile     = ScriptRoot / TEXT("Classes/TestQ_Wedge.as");
    const auto CanonicalFile = ScriptRoot / TEXT("Generated/FakeProj_EntitySpawnParams.as");
    const auto SiblingFile   = ScriptRoot / TEXT("Generated/_StubRecovery_FakeProj_EntitySpawnParams.as");

    IFileManager::Get().MakeDirectory(*FPaths::GetPath(ClassFile),     /*Tree=*/true);
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(CanonicalFile), /*Tree=*/true);
    FFileHelper::SaveStringToFile(FString{TEXT("{}")}, *(TempRoot / TEXT("FakeProj.uproject")));

    FFileHelper::SaveStringToFile(FString{TEXT(
        "class UTestQ_Wedge : UCk_GenericEntityScript_UE\n"
        "{\n"
        "    UPROPERTY(ExposeOnSpawn)\n"
        "    FCk_Handle StoreEntity;\n"
        "    UPROPERTY(ExposeOnSpawn)\n"
        "    int32 Knob;\n"
        "}\n")}, *ClassFile, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    FFileHelper::SaveStringToFile(FString{TEXT(
        "USTRUCT()\n"
        "struct FTestQ_Wedge_SpawnParams\n"
        "{\n"
        "}\n"
        "namespace UTestQ_Wedge\n"
        "{\n"
        "    FTestQ_Wedge_SpawnParams Params() { return FTestQ_Wedge_SpawnParams(); }\n"
        "}\n")}, *CanonicalFile, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    // The wedged leftover: an Arg0..ArgN error-text stub typed from ONE call site, which
    // cannot satisfy mixed-type callers.
    FFileHelper::SaveStringToFile(FString{TEXT(
        "// stub header\n"
        "namespace UTestQ_Wedge\n"
        "{\n"
        "    FTestQ_Wedge_SpawnParams Params(FCk_Handle_Economy Arg0, int Arg1)\n"
        "    {\n"
        "        return FTestQ_Wedge_SpawnParams();\n"
        "    }\n"
        "}\n"
        "// End synthesized stub for UTestQ_Wedge::Params(FCk_Handle_Economy, int)\n")},
        *SiblingFile, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    const auto Error = Make_ParsedError(
        TEXT("UTestQ_Wedge"), TEXT("Params"), TEXT("FCk_Handle, const int"));

    const auto Result = FCkAsStubSynthesizer::Quarantine_And_ResynthesizeFullShapes(
        CanonicalFile, {TEXT("UTestQ_Wedge")}, Error, {ScriptRoot});

    TestTrue(TEXT("quarantine succeeded"), Result.Success);

    auto Stub = FString{};
    TestTrue(TEXT("sibling rebuilt"), FFileHelper::LoadFileToString(Stub, *SiblingFile));
    TestFalse(TEXT("wedged Arg0-style error-text stub is gone"),
        Stub.Contains(TEXT("Arg0")));
    TestFalse(TEXT("the one-caller Economy-typed param is gone"),
        Stub.Contains(TEXT("FCk_Handle_Economy")));
    TestTrue(TEXT("exact-typed full shape landed (named In<Prop> params)"),
        Stub.Contains(TEXT("FTestQ_Wedge_SpawnParams(FCk_Handle InStoreEntity, int32 InKnob)")));
    TestEqual(TEXT("struct defined exactly once"),
        Count_Occurrences(Stub, TEXT("struct FTestQ_Wedge_SpawnParams")), 1);

    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Deleting the canonical removes EVERY class's struct, so rebuilding only the erroring one
// would burn a bootstrap cycle per remaining class.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StubSynthesizer_Quarantine_SynthesizesUnionOfCanonicalClasses,
    "CkAngelscriptGenerator.UnitTests.StubSynthesizer.Quarantine_SynthesizesUnionOfCanonicalClasses",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StubSynthesizer_Quarantine_SynthesizesUnionOfCanonicalClasses::RunTest(const FString&)
{
    FCkAsStubSynthesizer::Reset_SessionState_ForTests();

    const auto TempRoot = FPaths::ConvertRelativePathToFull(
        FPaths::ProjectIntermediateDir() / TEXT("CkStubSynthTest_QuarantineUnion"));
    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);

    const auto ScriptRoot    = TempRoot / TEXT("Script");
    const auto CanonicalFile = ScriptRoot / TEXT("Generated/FakeProj_EntitySpawnParams.as");
    const auto SiblingFile   = ScriptRoot / TEXT("Generated/_StubRecovery_FakeProj_EntitySpawnParams.as");

    IFileManager::Get().MakeDirectory(*(ScriptRoot / TEXT("Classes")), /*Tree=*/true);
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(CanonicalFile), /*Tree=*/true);
    FFileHelper::SaveStringToFile(FString{TEXT("{}")}, *(TempRoot / TEXT("FakeProj.uproject")));

    FFileHelper::SaveStringToFile(FString{TEXT(
        "class UTestQ_UnionA : UCk_GenericEntityScript_UE\n"
        "{\n"
        "    UPROPERTY(ExposeOnSpawn)\n"
        "    int32 A;\n"
        "}\n")}, *(ScriptRoot / TEXT("Classes/TestQ_UnionA.as")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    FFileHelper::SaveStringToFile(FString{TEXT(
        "class UTestQ_UnionB : UCk_GenericEntityScript_UE\n"
        "{\n"
        "    UPROPERTY(ExposeOnSpawn)\n"
        "    int32 B;\n"
        "}\n")}, *(ScriptRoot / TEXT("Classes/TestQ_UnionB.as")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    FFileHelper::SaveStringToFile(FString{TEXT(
        "struct FTestQ_UnionA_SpawnParams\n"
        "{\n"
        "}\n"
        "namespace UTestQ_UnionA\n"
        "{\n"
        "}\n"
        "struct FTestQ_UnionB_SpawnParams\n"
        "{\n"
        "}\n"
        "namespace UTestQ_UnionB\n"
        "{\n"
        "}\n")}, *CanonicalFile, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    const auto Error = Make_ParsedError(TEXT("UTestQ_UnionA"), TEXT("Params"), TEXT("const int"));

    const auto Result = FCkAsStubSynthesizer::Quarantine_And_ResynthesizeFullShapes(
        CanonicalFile, {TEXT("UTestQ_UnionA")}, Error, {ScriptRoot});

    TestTrue(TEXT("quarantine succeeded"), Result.Success);

    auto Stub = FString{};
    TestTrue(TEXT("sibling rebuilt"), FFileHelper::LoadFileToString(Stub, *SiblingFile));
    TestTrue(TEXT("seed class UTestQ_UnionA synthesized"),
        Stub.Contains(TEXT("namespace UTestQ_UnionA")));
    TestTrue(TEXT("non-seed canonical class UTestQ_UnionB synthesized too"),
        Stub.Contains(TEXT("namespace UTestQ_UnionB")));

    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StubSynthesizer_Quarantine_SkipsClassesWithNoSource,
    "CkAngelscriptGenerator.UnitTests.StubSynthesizer.Quarantine_SkipsClassesWithNoSource",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StubSynthesizer_Quarantine_SkipsClassesWithNoSource::RunTest(const FString&)
{
    FCkAsStubSynthesizer::Reset_SessionState_ForTests();

    const auto TempRoot = FPaths::ConvertRelativePathToFull(
        FPaths::ProjectIntermediateDir() / TEXT("CkStubSynthTest_QuarantineSkip"));
    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);

    const auto ScriptRoot    = TempRoot / TEXT("Script");
    const auto CanonicalFile = ScriptRoot / TEXT("Generated/FakeProj_EntitySpawnParams.as");
    const auto SiblingFile   = ScriptRoot / TEXT("Generated/_StubRecovery_FakeProj_EntitySpawnParams.as");

    IFileManager::Get().MakeDirectory(*(ScriptRoot / TEXT("Classes")), /*Tree=*/true);
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(CanonicalFile), /*Tree=*/true);
    FFileHelper::SaveStringToFile(FString{TEXT("{}")}, *(TempRoot / TEXT("FakeProj.uproject")));

    FFileHelper::SaveStringToFile(FString{TEXT(
        "class UTestQ_Live : UCk_GenericEntityScript_UE\n"
        "{\n"
        "    UPROPERTY(ExposeOnSpawn)\n"
        "    int32 Live;\n"
        "}\n")}, *(ScriptRoot / TEXT("Classes/TestQ_Live.as")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    // UTestQ_Gone has NO source file — a deleted class surviving in the stale canonical.
    FFileHelper::SaveStringToFile(FString{TEXT(
        "struct FTestQ_Live_SpawnParams\n"
        "{\n"
        "}\n"
        "namespace UTestQ_Live\n"
        "{\n"
        "}\n"
        "struct FTestQ_Gone_SpawnParams\n"
        "{\n"
        "}\n"
        "namespace UTestQ_Gone\n"
        "{\n"
        "}\n")}, *CanonicalFile, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    const auto Error = Make_ParsedError(TEXT("UTestQ_Live"), TEXT("Params"), TEXT("const int"));

    const auto Result = FCkAsStubSynthesizer::Quarantine_And_ResynthesizeFullShapes(
        CanonicalFile, {TEXT("UTestQ_Live")}, Error, {ScriptRoot});

    TestTrue(TEXT("quarantine succeeded despite the unscannable non-seed class"), Result.Success);
    TestTrue(TEXT("skip is reported in the message"),
        Result.ErrorMessage.Contains(TEXT("skipped")));

    auto Stub = FString{};
    TestTrue(TEXT("sibling rebuilt"), FFileHelper::LoadFileToString(Stub, *SiblingFile));
    TestTrue(TEXT("live class synthesized"),  Stub.Contains(TEXT("namespace UTestQ_Live")));
    TestFalse(TEXT("gone class absent"),      Stub.Contains(TEXT("namespace UTestQ_Gone")));

    FCkAsStubSynthesizer::Reset_SessionState_ForTests();
    FFileHelper::SaveStringToFile(FString{TEXT(
        "struct FTestQ_Gone_SpawnParams\n"
        "{\n"
        "}\n"
        "namespace UTestQ_Gone\n"
        "{\n"
        "}\n")}, *CanonicalFile, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    const auto SeedGone = FCkAsStubSynthesizer::Quarantine_And_ResynthesizeFullShapes(
        CanonicalFile, {TEXT("UTestQ_Gone")}, Error, {ScriptRoot});
    TestFalse(TEXT("unscannable SEED fails the escalation"), SeedGone.Success);
    TestTrue(TEXT("seed failure reason is ScanFailed"),
        SeedGone.FailReason == ECk_StubInjectFailReason::ScanFailed);

    IFileManager::Get().DeleteDirectory(*TempRoot, /*RequireExists=*/false, /*Tree=*/true);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
