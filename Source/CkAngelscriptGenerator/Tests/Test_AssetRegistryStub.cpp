// Tests for the AssetRegistry stub synthesizer (Rev 10 dispatcher recovery strategy #3).
//
// Coverage focuses on the pure-string and pure-logic surfaces:
//   * Classify_AccessorFlavor       — namespace + function-name shape disambiguation
//   * Strip_LoadSuffix              — base-namespace derivation
//   * Strip_ClassSuffix             — base-function-name derivation
//   * Build_SoftRefAccessor         — soft-ref accessor body shape
//   * Build_SoftClassAccessor       — soft-class accessor body shape + "_C" suffix
//   * Build_BlockingLoadAccessor    — blocking-load accessor body shape + ensure guard
//   * Build_NamespaceBlock          — namespace wrapper + marker comments
//
// Inject_AssetRegistryStub is the live entry point and exercises engine state
// (AR scan, sync asset load, file IO). Coverage of that path comes from the
// end-to-end smoke test via _probe_assets_corrupt.bat — same precedent as the
// EntitySpawnParams synthesizer (Test_StubSynthesizer.cpp has no live-engine
// tests either).

#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AssetRegistryStub.h"
#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AsErrorParser.h"

#include "CkCore/Macros/CkMacros.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

using namespace ck::angelscriptgenerator::self_heal;

namespace
{
    auto Make_AssetParsedError(
        const TCHAR* InTargetNamespace,
        const TCHAR* InFunctionName,
        const TCHAR* InArgsList   = TEXT(""),
        const TCHAR* InFilePath   = TEXT("D:/Test/Caller.as"),
        int32        InLine       = 100,
        int32        InColumn     = 5) -> FCk_AsParsedError
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
// Classify_AccessorFlavor: namespace + function-name shape disambiguation.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetRegistryStub_Classify_Flavor,
    "CkAngelscriptGenerator.UnitTests.AssetRegistryStub.Classify_Flavor",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetRegistryStub_Classify_Flavor::RunTest(const FString&)
{
    TestEqual(TEXT("assets::FOO() -> SoftRef"),
        static_cast<int32>(FCkAsAssetRegistryStubSynthesizer::Classify_AccessorFlavor(
            Make_AssetParsedError(TEXT("assets"), TEXT("MALE_SKEL_NEW")))),
        static_cast<int32>(ECk_AssetAccessorFlavor::SoftRef));

    TestEqual(TEXT("assets::load::FOO() -> BlockingLoad"),
        static_cast<int32>(FCkAsAssetRegistryStubSynthesizer::Classify_AccessorFlavor(
            Make_AssetParsedError(TEXT("assets::load"), TEXT("MALE_SKEL_NEW")))),
        static_cast<int32>(ECk_AssetAccessorFlavor::BlockingLoad));

    TestEqual(TEXT("assets::FOO_Class() -> SoftClass"),
        static_cast<int32>(FCkAsAssetRegistryStubSynthesizer::Classify_AccessorFlavor(
            Make_AssetParsedError(TEXT("assets"), TEXT("MyActor_BP_Class")))),
        static_cast<int32>(ECk_AssetAccessorFlavor::SoftClass));

    // BlockingLoad wins over SoftClass when both could apply (assets::load::FOO_Class
    // is the blocking variant of the soft-class accessor — TSubclassOf returning).
    TestEqual(TEXT("assets::load::FOO_Class() -> BlockingLoad (load wins)"),
        static_cast<int32>(FCkAsAssetRegistryStubSynthesizer::Classify_AccessorFlavor(
            Make_AssetParsedError(TEXT("assets::load"), TEXT("MyActor_BP_Class")))),
        static_cast<int32>(ECk_AssetAccessorFlavor::BlockingLoad));

    // Plugin-scoped namespace ("game_assets" etc.) follows the same rules.
    TestEqual(TEXT("game_assets::load::FOO() -> BlockingLoad"),
        static_cast<int32>(FCkAsAssetRegistryStubSynthesizer::Classify_AccessorFlavor(
            Make_AssetParsedError(TEXT("game_assets::load"), TEXT("FOO")))),
        static_cast<int32>(ECk_AssetAccessorFlavor::BlockingLoad));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Strip_LoadSuffix / Strip_ClassSuffix: string-trimming helpers.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetRegistryStub_Strip_LoadSuffix,
    "CkAngelscriptGenerator.UnitTests.AssetRegistryStub.Strip_LoadSuffix",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetRegistryStub_Strip_LoadSuffix::RunTest(const FString&)
{
    TestEqual(TEXT("assets::load -> assets"),
        FCkAsAssetRegistryStubSynthesizer::Strip_LoadSuffix(TEXT("assets::load")),
        FString{TEXT("assets")});

    TestEqual(TEXT("game_assets::load -> game_assets"),
        FCkAsAssetRegistryStubSynthesizer::Strip_LoadSuffix(TEXT("game_assets::load")),
        FString{TEXT("game_assets")});

    TestEqual(TEXT("assets (no suffix) -> assets"),
        FCkAsAssetRegistryStubSynthesizer::Strip_LoadSuffix(TEXT("assets")),
        FString{TEXT("assets")});

    TestEqual(TEXT("empty -> empty"),
        FCkAsAssetRegistryStubSynthesizer::Strip_LoadSuffix(FString{}),
        FString{});

    // Defensive: don't trim "load" if it isn't preceded by "::".
    TestEqual(TEXT("preload (not :: load) -> preload"),
        FCkAsAssetRegistryStubSynthesizer::Strip_LoadSuffix(TEXT("preload")),
        FString{TEXT("preload")});

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetRegistryStub_Strip_ClassSuffix,
    "CkAngelscriptGenerator.UnitTests.AssetRegistryStub.Strip_ClassSuffix",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetRegistryStub_Strip_ClassSuffix::RunTest(const FString&)
{
    TestEqual(TEXT("MyActor_BP_Class -> MyActor_BP"),
        FCkAsAssetRegistryStubSynthesizer::Strip_ClassSuffix(TEXT("MyActor_BP_Class")),
        FString{TEXT("MyActor_BP")});

    TestEqual(TEXT("FOO (no suffix) -> FOO"),
        FCkAsAssetRegistryStubSynthesizer::Strip_ClassSuffix(TEXT("FOO")),
        FString{TEXT("FOO")});

    TestEqual(TEXT("empty -> empty"),
        FCkAsAssetRegistryStubSynthesizer::Strip_ClassSuffix(FString{}),
        FString{});

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Build_SoftRefAccessor: TSoftObjectPtr<X> shape.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetRegistryStub_Build_SoftRef,
    "CkAngelscriptGenerator.UnitTests.AssetRegistryStub.Build_SoftRef",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetRegistryStub_Build_SoftRef::RunTest(const FString&)
{
    const auto Body = FCkAsAssetRegistryStubSynthesizer::Build_SoftRefAccessor(
        TEXT("MALE_SKEL_NEW"),
        TEXT("USkeletalMesh"),
        TEXT("/Game/Raw/SKM/MALE_SKEL_NEW.MALE_SKEL_NEW"));

    TestTrue(TEXT("contains return type"), Body.Contains(TEXT("TSoftObjectPtr<USkeletalMesh>")));
    TestTrue(TEXT("contains function name"), Body.Contains(TEXT("MALE_SKEL_NEW()")));
    TestTrue(TEXT("contains FSoftObjectPath wrapper"), Body.Contains(TEXT("FSoftObjectPath(\"/Game/Raw/SKM/MALE_SKEL_NEW.MALE_SKEL_NEW\")")));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Build_SoftClassAccessor: TSoftClassPtr<X> shape, with "_C" appended to AssetPath.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetRegistryStub_Build_SoftClass,
    "CkAngelscriptGenerator.UnitTests.AssetRegistryStub.Build_SoftClass",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetRegistryStub_Build_SoftClass::RunTest(const FString&)
{
    const auto Body = FCkAsAssetRegistryStubSynthesizer::Build_SoftClassAccessor(
        TEXT("MyActor_BP_Class"),
        TEXT("AMyActor"),
        TEXT("/Game/BP/MyActor_BP.MyActor_BP"));

    TestTrue(TEXT("contains TSoftClassPtr"), Body.Contains(TEXT("TSoftClassPtr<AMyActor>")));
    TestTrue(TEXT("contains Class suffix in function name"), Body.Contains(TEXT("MyActor_BP_Class()")));
    TestTrue(TEXT("appends _C to asset path"),
        Body.Contains(TEXT("FSoftObjectPath(\"/Game/BP/MyActor_BP.MyActor_BP_C\")")));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Build_BlockingLoadAccessor: engine-init guard + LoadAsset_Blocking delegation.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetRegistryStub_Build_BlockingLoad,
    "CkAngelscriptGenerator.UnitTests.AssetRegistryStub.Build_BlockingLoad",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetRegistryStub_Build_BlockingLoad::RunTest(const FString&)
{
    const auto Body = FCkAsAssetRegistryStubSynthesizer::Build_BlockingLoadAccessor(
        TEXT("MALE_SKEL_NEW"),
        TEXT("USkeletalMesh"),
        TEXT("assets"));

    TestTrue(TEXT("contains return type (no Soft wrapper)"), Body.Contains(TEXT("USkeletalMesh MALE_SKEL_NEW()")));
    TestTrue(TEXT("contains engine-init ensure"), Body.Contains(TEXT("UCk_Utils_IO_UE::IsEngineSafeForBlockingLoads")));
    TestTrue(TEXT("contains nullptr early return"), Body.Contains(TEXT("return nullptr;")));
    TestTrue(TEXT("delegates to soft accessor"), Body.Contains(TEXT("System::LoadAsset_Blocking(assets::MALE_SKEL_NEW())")));
    // Error message correctly references both namespaces.
    TestTrue(TEXT("ensure message references assets::load::FOO"), Body.Contains(TEXT("assets::load::MALE_SKEL_NEW()")));
    TestTrue(TEXT("ensure message references assets::FOO (soft)"), Body.Contains(TEXT("Use assets::MALE_SKEL_NEW() (soft ref)")));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Build_NamespaceBlock: marker comment + namespace wrapper around function body.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetRegistryStub_Build_NamespaceBlock,
    "CkAngelscriptGenerator.UnitTests.AssetRegistryStub.Build_NamespaceBlock",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetRegistryStub_Build_NamespaceBlock::RunTest(const FString&)
{
    const auto Error = Make_AssetParsedError(TEXT("assets"), TEXT("MALE_SKEL_NEW"), TEXT(""),
        TEXT("D:/Repos/BB/Script/Foo.as"), 42, 7);
    const auto Body  = FCkAsAssetRegistryStubSynthesizer::Build_SoftRefAccessor(
        TEXT("MALE_SKEL_NEW"), TEXT("USkeletalMesh"),
        TEXT("/Game/Raw/SKM/MALE_SKEL_NEW.MALE_SKEL_NEW"));

    const auto Block = FCkAsAssetRegistryStubSynthesizer::Build_NamespaceBlock(
        TEXT("assets"), Body, Error);

    TestTrue(TEXT("has marker comment"), Block.Contains(FCkAsAssetRegistryStubSynthesizer::Get_MarkerComment()));
    TestTrue(TEXT("has target comment"),  Block.Contains(TEXT("// Target: assets::MALE_SKEL_NEW()")));
    TestTrue(TEXT("has triggering site"), Block.Contains(TEXT("D:/Repos/BB/Script/Foo.as:42:7")));
    TestTrue(TEXT("opens namespace"),     Block.Contains(TEXT("namespace assets")));
    TestTrue(TEXT("contains body"),       Block.Contains(TEXT("TSoftObjectPtr<USkeletalMesh>")));
    TestTrue(TEXT("has closing brace"),   Block.Contains(TEXT("}")));
    TestTrue(TEXT("has end-stub comment"), Block.Contains(TEXT("// End synthesized stub for assets::MALE_SKEL_NEW")));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Marker comment is non-empty and uniquely identifies AssetRegistry stubs.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetRegistryStub_MarkerComment,
    "CkAngelscriptGenerator.UnitTests.AssetRegistryStub.MarkerComment",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetRegistryStub_MarkerComment::RunTest(const FString&)
{
    const auto Marker = FCkAsAssetRegistryStubSynthesizer::Get_MarkerComment();

    TestFalse(TEXT("not empty"), Marker.IsEmpty());
    TestTrue(TEXT("starts with comment"), Marker.StartsWith(TEXT("//")));
    TestTrue(TEXT("identifies as AssetRegistry"), Marker.Contains(TEXT("AssetRegistry")));
    TestTrue(TEXT("identifies as synthesized/emergency"), Marker.Contains(TEXT("synthesized")));

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------
