// Tests for the AssetRegistry stub synthesizer (Rev 10 strategy #3).
//
// Pure-logic surfaces (Classify_*, Strip_*, Build_*, header parsing, site
// picking). The live Inject_AssetRegistryStub entry point isn't unit-tested
// — it exercises engine state (LoadObject, file IO); coverage there comes
// from the `_probe_assets_corrupt.bat` end-to-end smoke runs.

#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AssetRegistryStub.h"
#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AsErrorParser.h"

#include "CkCore/Macros/CkMacros.h"

#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"

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

    // Blocking variant of the soft-class accessor — its own flavor. Plain
    // BlockingLoad would skip `_Class` strip; plain SoftClass would emit
    // TSoftClassPtr instead of the blocking body.
    TestEqual(TEXT("assets::load::FOO_Class() -> BlockingLoadClass (load + class both)"),
        static_cast<int32>(FCkAsAssetRegistryStubSynthesizer::Classify_AccessorFlavor(
            Make_AssetParsedError(TEXT("assets::load"), TEXT("MyActor_BP_Class")))),
        static_cast<int32>(ECk_AssetAccessorFlavor::BlockingLoadClass));

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
// Build_BlockingLoadClassAccessor: engine-init guard + LoadClassAsset_Blocking
// delegation, TSubclassOf<Class> return shape.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetRegistryStub_Build_BlockingLoadClass,
    "CkAngelscriptGenerator.UnitTests.AssetRegistryStub.Build_BlockingLoadClass",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetRegistryStub_Build_BlockingLoadClass::RunTest(const FString&)
{
    const auto Body = FCkAsAssetRegistryStubSynthesizer::Build_BlockingLoadClassAccessor(
        TEXT("MyActor_BP_Class"),
        TEXT("AActor"),
        TEXT("assets"));

    TestTrue(TEXT("contains TSubclassOf return shape"), Body.Contains(TEXT("TSubclassOf<AActor> MyActor_BP_Class()")));
    TestTrue(TEXT("contains engine-init ensure"), Body.Contains(TEXT("UCk_Utils_IO_UE::IsEngineSafeForBlockingLoads")));
    TestTrue(TEXT("contains nullptr early return"), Body.Contains(TEXT("return nullptr;")));
    TestTrue(TEXT("delegates to LoadClassAsset_Blocking with soft-class accessor"),
        Body.Contains(TEXT("System::LoadClassAsset_Blocking(assets::MyActor_BP_Class())")));
    TestTrue(TEXT("ensure message references assets::load::FOO_Class"),
        Body.Contains(TEXT("assets::load::MyActor_BP_Class()")));
    TestTrue(TEXT("ensure message references assets::FOO_Class (soft)"),
        Body.Contains(TEXT("Use assets::MyActor_BP_Class() (soft ref)")));

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

// --------------------------------------------------------------------------------------------------------------------
// Output-site disambiguation: when multiple *Assets.as files share `namespace
// assets` but cover disjoint DiscoveryRoots (BusterBlockAssets.as on
// /Game/BusterBlock/, RawAssets.as on /Game/Raw/), the synthesizer must pick
// the file whose root prefixes the failing asset's package path — not the
// first-alphabetical match. Fixture: write two fake *Assets.as files into a
// temp dir, run Collect_MatchingSites + Pick_BestSite_ByAssetPath against
// them, assert the right one is selected.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetRegistryStub_PickSite_ByAssetPath,
    "CkAngelscriptGenerator.UnitTests.AssetRegistryStub.PickSite_ByAssetPath",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetRegistryStub_PickSite_ByAssetPath::RunTest(const FString&)
{
    // Build a transient fixture directory under the engine's intermediate area
    // so it survives even if ProjectIntermediateDir isn't accessible in this
    // automation context. Cleanup happens on scope exit regardless of outcome.
    const auto FixtureDir = FPaths::Combine(
        FPaths::EngineIntermediateDir(),
        TEXT("CkAsAssetRegistryStub_Tests"),
        FGuid::NewGuid().ToString(EGuidFormats::Short));

    if (NOT IFileManager::Get().MakeDirectory(*FixtureDir, /*Tree=*/true))
    {
        AddError(FString::Printf(TEXT("Could not create fixture dir '%s'"), *FixtureDir));
        return false;
    }

    ON_SCOPE_EXIT
    { IFileManager::Get().DeleteDirectory(*FixtureDir, /*RequireExists=*/false, /*Tree=*/true); };

    // Two files, same namespace, distinct discovery roots. Mirror the real
    // generator's header shape exactly.
    const auto BusterBlockFile = FPaths::Combine(FixtureDir, TEXT("BusterBlockAssets.as"));
    const auto RawFile         = FPaths::Combine(FixtureDir, TEXT("RawAssets.as"));

    // Mirror the real generator's malformed `Source config:` token (missing
    // slash before the .as filename) for the BB file — this is the actual
    // shape we have to be robust against, per the 2026-05-12 smoke test.
    // The canonical root is on the separate `Discovery root:` line.
    const auto BusterBlockBody = FString{TEXT(
        "// Auto-generated Asset Registry\r\n")
        TEXT("// Source config: BusterBlockAssets (/Game/BusterBlockBusterBlockAssets.as [assets])\r\n")
        TEXT("// Discovery root: /Game/BusterBlock\r\n")
        TEXT("namespace assets { }\r\n")};
    const auto RawBody = FString{TEXT(
        "// Auto-generated Asset Registry\r\n")
        TEXT("// Source config: RawAssets (/Game/Raw/RawAssets.as [assets])\r\n")
        TEXT("// Discovery root: /Game/Raw/\r\n")
        TEXT("namespace assets { }\r\n")};

    if (NOT FFileHelper::SaveStringToFile(BusterBlockBody, *BusterBlockFile)
        || NOT FFileHelper::SaveStringToFile(RawBody, *RawFile))
    {
        AddError(TEXT("Could not write fixture files"));
        return false;
    }

    // ---- Header parse (single file) ------------------------------------------------

    {
        auto Site = ck::angelscriptgenerator::self_heal::FCk_AssetConfigSiteInfo{};
        auto Ns   = FString{};
        TestTrue(TEXT("parses RawAssets header"),
            FCkAsAssetRegistryStubSynthesizer::Try_ParseConfigSiteHeader(RawFile, Site, Ns));
        TestEqual(TEXT("Raw namespace"),     Ns,                  FString{TEXT("assets")});
        TestEqual(TEXT("Raw discovery root"), Site.DiscoveryRoot, FString{TEXT("/Game/Raw/")});
        TestEqual(TEXT("Raw output path"),    Site.OutputPath,    RawFile);
    }

    // ---- Collect_MatchingSites (both files) ---------------------------------------

    const auto Dirs = TArray<FString>{FixtureDir};
    const auto Sites = FCkAsAssetRegistryStubSynthesizer::Collect_MatchingSites(Dirs, TEXT("assets"));
    TestEqual(TEXT("both files collected"), Sites.Num(), 2);

    // ---- Pick_BestSite_ByAssetPath ------------------------------------------------

    // The bug case: failing asset lives under /Game/Raw/ — the RawAssets site
    // must win even though BusterBlockAssets.as sorts first alphabetically.
    {
        const auto PickedIdx = FCkAsAssetRegistryStubSynthesizer::Pick_BestSite_ByAssetPath(
            Sites, TEXT("/Game/Raw/SKM/MALE_SKEL_NEW.MALE_SKEL_NEW"));
        TestTrue(TEXT("Raw asset picks some candidate"), PickedIdx != INDEX_NONE);
        if (PickedIdx != INDEX_NONE)
        {
            TestEqual(TEXT("Raw asset picks /Game/Raw/ root"),
                Sites[PickedIdx].DiscoveryRoot, FString{TEXT("/Game/Raw/")});
        }
    }

    // The symmetric case: asset lives under /Game/BusterBlock/.
    {
        const auto PickedIdx = FCkAsAssetRegistryStubSynthesizer::Pick_BestSite_ByAssetPath(
            Sites, TEXT("/Game/BusterBlock/Char/Foo.Foo"));
        TestTrue(TEXT("BB asset picks some candidate"), PickedIdx != INDEX_NONE);
        if (PickedIdx != INDEX_NONE)
        {
            TestEqual(TEXT("BB asset picks /Game/BusterBlock/ root"),
                Sites[PickedIdx].DiscoveryRoot, FString{TEXT("/Game/BusterBlock/")});
        }
    }

    // No candidate's root prefixes the asset path -> INDEX_NONE (caller's
    // first-match fallback then kicks in).
    {
        const auto PickedIdx = FCkAsAssetRegistryStubSynthesizer::Pick_BestSite_ByAssetPath(
            Sites, TEXT("/Engine/Foo/Bar.Bar"));
        TestTrue(TEXT("unrelated path -> INDEX_NONE"), PickedIdx == INDEX_NONE);
    }

    // Empty asset path -> INDEX_NONE (defensive).
    {
        const auto PickedIdx = FCkAsAssetRegistryStubSynthesizer::Pick_BestSite_ByAssetPath(
            Sites, FString{});
        TestTrue(TEXT("empty path -> INDEX_NONE"), PickedIdx == INDEX_NONE);
    }

    // Longest-prefix wins: synthesize a 3-candidate set with nested roots.
    {
        auto Nested = TArray<ck::angelscriptgenerator::self_heal::FCk_AssetConfigSiteInfo>{};
        {
            auto S = ck::angelscriptgenerator::self_heal::FCk_AssetConfigSiteInfo{};
            S.OutputPath = TEXT("a"); S.DiscoveryRoot = TEXT("/Game/"); Nested.Add(S);
        }
        {
            auto S = ck::angelscriptgenerator::self_heal::FCk_AssetConfigSiteInfo{};
            S.OutputPath = TEXT("b"); S.DiscoveryRoot = TEXT("/Game/Raw/"); Nested.Add(S);
        }
        {
            auto S = ck::angelscriptgenerator::self_heal::FCk_AssetConfigSiteInfo{};
            S.OutputPath = TEXT("c"); S.DiscoveryRoot = TEXT("/Other/"); Nested.Add(S);
        }
        const auto Idx = FCkAsAssetRegistryStubSynthesizer::Pick_BestSite_ByAssetPath(
            Nested, TEXT("/Game/Raw/SKM/X.X"));
        TestEqual(TEXT("longest-prefix root wins"),
            Nested.IsValidIndex(Idx) ? Nested[Idx].DiscoveryRoot : FString{},
            FString{TEXT("/Game/Raw/")});
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Derive_StubSiblingPath: AssetRegistry variant — same convention.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetRegistryStub_DeriveStubSiblingPath,
    "CkAngelscriptGenerator.UnitTests.AssetRegistryStub.DeriveStubSiblingPath",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetRegistryStub_DeriveStubSiblingPath::RunTest(const FString&)
{
    TestEqual(TEXT("BusterBlockAssets.as -> _StubRecovery_BusterBlockAssets.as"),
        FCkAsAssetRegistryStubSynthesizer::Derive_StubSiblingPath(
            TEXT("D:/Repos/BB/Script/Generated/BusterBlockAssets.as")),
        FString{TEXT("D:/Repos/BB/Script/Generated/_StubRecovery_BusterBlockAssets.as")});

    TestEqual(TEXT("empty -> empty"),
        FCkAsAssetRegistryStubSynthesizer::Derive_StubSiblingPath(FString{}),
        FString{});

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Stub-file recovery header banner is present + identifies the file.
// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AssetRegistryStub_StubFileHeader,
    "CkAngelscriptGenerator.UnitTests.AssetRegistryStub.StubFileHeader",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AssetRegistryStub_StubFileHeader::RunTest(const FString&)
{
    const auto Header = FCkAsAssetRegistryStubSynthesizer::Get_StubFileHeader();
    TestFalse(TEXT("not empty"), Header.IsEmpty());
    TestTrue(TEXT("contains AUTO-GENERATED RECOVERY STUBS"),
        Header.Contains(TEXT("AUTO-GENERATED RECOVERY STUBS")));
    TestTrue(TEXT("notes GITIGNORED status"), Header.Contains(TEXT("GITIGNORED")));
    TestTrue(TEXT("notes self-cleans"),       Header.Contains(TEXT("self-cleans")));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------
