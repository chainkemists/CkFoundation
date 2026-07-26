#pragma once

#include "CoreMinimal.h"

#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AsErrorParser.h"

// --------------------------------------------------------------------------------------------------------------------

// Emergency stub synthesizer for AssetRegistry accessor drift: emits the
// missing `assets::*` accessor into a sibling `_StubRecovery_<MatchedAssetsFile>.as`
// (canonicals are never touched; AS namespace-merge satisfies the compile).
// Discovery is deliberately AssetRegistry-free and class resolution is shared
// with the canonical generator — see CkAngelscriptGenerator/CLAUDE.md.

namespace ck::angelscriptgenerator::self_heal
{
    struct CKANGELSCRIPTGENERATOR_API FCk_AssetStubInjectionResult
    {
        bool    Success = false;
        FString TargetFilePath;       // sibling stub file (empty on failure)
        FString InjectedBlock;        // verbatim stub text (empty on failure)
        FString ResolvedAssetClass;   // e.g. "USkeletalMesh" (empty on failure)
        FString ResolvedAssetPath;    // e.g. "/Game/Raw/SKM/MALE_SKEL_NEW.MALE_SKEL_NEW"
        FString ErrorMessage;         // populated on failure
        bool    UsedTier3Fallback = false; // dead — Tier 3 is refused; retained for source compat
    };

    // `assets::FOO()`              -> SoftRef
    // `assets::load::FOO()`        -> BlockingLoad
    // `assets::FOO_Class()`        -> SoftClass        (BP-only)
    // `assets::load::FOO_Class()`  -> BlockingLoadClass (BP-only — blocking variant of SoftClass)
    enum class ECk_AssetAccessorFlavor : uint8
    {
        SoftRef,
        BlockingLoad,
        SoftClass,
        BlockingLoadClass,
    };

    struct CKANGELSCRIPTGENERATOR_API FCk_AssetConfigSiteInfo
    {
        FString OutputPath;
        FString DiscoveryRoot;
    };

    class CKANGELSCRIPTGENERATOR_API FCkAsAssetRegistryStubSynthesizer
    {
    public:
        // ---- Pure-logic surface ----------------------------------------------------

        static auto Classify_AccessorFlavor(const FCk_AsParsedError& InError) -> ECk_AssetAccessorFlavor;
        static auto Strip_LoadSuffix       (const FString& InNamespace)       -> FString;
        static auto Strip_ClassSuffix      (const FString& InFunctionName)    -> FString;

        // `TSoftObjectPtr<UClass> FuncName() { return TSoftObjectPtr<UClass>(FSoftObjectPath("/Game/...")); }`
        static auto Build_SoftRefAccessor(
            const FString& InFunctionName,
            const FString& InResolvedClassName,
            const FString& InAssetPath) -> FString;

        // BP _Class variant. Appends `_C` to InAssetPath per AS convention.
        static auto Build_SoftClassAccessor(
            const FString& InFunctionName,
            const FString& InResolvedClassName,
            const FString& InAssetPath) -> FString;

        // Emits an ensure-guarded LoadAsset_Blocking delegating to the soft-ref.
        static auto Build_BlockingLoadAccessor(
            const FString& InFunctionName,
            const FString& InResolvedClassName,
            const FString& InSoftNamespace) -> FString;

        // BP _Class blocking variant; InFunctionName carries the `_Class` suffix.
        static auto Build_BlockingLoadClassAccessor(
            const FString& InFunctionName,
            const FString& InResolvedClassName,
            const FString& InSoftNamespace) -> FString;

        static auto Build_NamespaceBlock(
            const FString&           InNamespace,
            const FString&           InFunctionBody,
            const FCk_AsParsedError& InError) -> FString;

        static auto Get_MarkerComment    () -> FString;
        static auto Get_StubFileHeader   () -> FString;
        static auto Derive_StubSiblingPath(const FString& InCanonicalFilePath) -> FString;

        // The root comes from `// Discovery root:`, NOT from the malformed path
        // token on `// Source config:`. False if absent in the first ~10 lines.
        static auto Try_ParseConfigSiteHeader(
            const FString&            InFilePath,
            FCk_AssetConfigSiteInfo&  OutSite,
            FString&                  OutNamespace) -> bool;

        // Alphabetical order from FindFilesRecursive for determinism.
        static auto Collect_MatchingSites(
            const TArray<FString>& InDirs,
            const FString&         InNamespace) -> TArray<FCk_AssetConfigSiteInfo>;

        // Longest-prefix match wins ("/Game/Raw/" beats "/Game/"). INDEX_NONE
        // when no candidate root prefixes the asset path.
        static auto Pick_BestSite_ByAssetPath(
            const TArray<FCk_AssetConfigSiteInfo>& InCandidates,
            const FString&                         InAssetPackagePath) -> int32;

        // ---- Live entry point (requires engine state) -----------------------------

        // Returns Success=false with a populated ErrorMessage on every failure
        // path — including an unresolvable class, where refusing is deliberate.
        static auto Inject_AssetRegistryStub(const FCk_AsParsedError& InError) -> FCk_AssetStubInjectionResult;
    };
}

// --------------------------------------------------------------------------------------------------------------------
