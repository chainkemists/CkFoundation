#pragma once

#include "CoreMinimal.h"

#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AsErrorParser.h"

// --------------------------------------------------------------------------------------------------------------------

// Emergency stub synthesizer for AssetRegistry accessor drift (Rev 10 strategy #3).
//
// Drift: a teammate's regen of `Script/Generated/*Assets.as` doesn't get
// committed alongside the AS callers that reference the new accessors. AS
// compile fails (`No matching signatures to 'assets::MALE_SKEL_NEW()'`),
// editor wedges on the Hazelight modal.
//
// Recovery (sibling-file model): synthesize the stub into a sibling
// `_StubRecovery_<MatchedAssetsFile>.as` next to the canonical, gitignored,
// deleted by PostCompile after a successful compile. AS namespace-merge
// across files satisfies compile. Canonical files are never touched.
//
// Class resolution:
//   * Tier 1/2 — LoadObject + Get_NonBlueprintParentClass (walks UBlueprint
//     -> native parent for BPs).
//   * Tier 3 — REFUSED for all flavors as of 2026-05-13 (probe_a2.log).
//
//     The original 2026-05-12 policy emitted `TSoftObjectPtr<UObject>` stubs
//     on the assumption that the caller's typed-conversion error would point
//     at the right line. Probe a2 disproved that: the typed-conversion error
//     doesn't match either of FCkAsErrorParser's two recognized patterns, so
//     cycle 2 parses zero actionable roots and the editor wedges on the
//     terminal banner. Refusing keeps Hazelight's original `No matching
//     signatures` error visible — actionable for the user.
//
// Discovery is AR-free — at modal-tick during initial-compile-failure, AR's
// SearchAllAssets/GetAssetsByClass exit early on
// IsEngineStartupModuleLoadingComplete() == FALSE. We file-scan
// `Script/Generated/*Assets.as` for `// Discovery root:` headers instead.

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
        bool    UsedTier3Fallback = false; // dead since 2026-05-13; field retained for source compat
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

    // (file path, discovery root) pair parsed from a *Assets.as header.
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

        // Ensure-guarded LoadAsset_Blocking that delegates to the soft-ref.
        static auto Build_BlockingLoadAccessor(
            const FString& InFunctionName,
            const FString& InResolvedClassName,
            const FString& InSoftNamespace) -> FString;

        // BP _Class blocking variant. Ensure-guarded LoadClassAsset_Blocking
        // that delegates to the soft-class accessor. InFunctionName carries
        // the `_Class` suffix; the emitted body returns TSubclassOf<Class>.
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

        // Reads `// Discovery root:` for the root (canonical source — the
        // `// Source config:` line was malformed by a U+2192 arrow concat
        // bug before commit c408ee8be). Returns false if header isn't
        // present in the first ~10 lines.
        static auto Try_ParseConfigSiteHeader(
            const FString&            InFilePath,
            FCk_AssetConfigSiteInfo&  OutSite,
            FString&                  OutNamespace) -> bool;

        // Alphabetical order from FindFilesRecursive for determinism.
        static auto Collect_MatchingSites(
            const TArray<FString>& InDirs,
            const FString&         InNamespace) -> TArray<FCk_AssetConfigSiteInfo>;

        // Longest-prefix match wins ("/Game/Raw/" beats "/Game/" for an
        // asset under "/Game/Raw/SKM/"). INDEX_NONE if no candidate root
        // prefixes the asset path.
        static auto Pick_BestSite_ByAssetPath(
            const TArray<FCk_AssetConfigSiteInfo>& InCandidates,
            const FString&                         InAssetPackagePath) -> int32;

        // ---- Live entry point (requires engine state) -----------------------------

        // Returns Success=false with a populated ErrorMessage on any failure
        // path. Tier 3 is REFUSED (see file-header rationale); the dispatcher
        // logs the message and Hazelight's modal keeps showing the original
        // actionable AS error.
        static auto Inject_AssetRegistryStub(const FCk_AsParsedError& InError) -> FCk_AssetStubInjectionResult;
    };
}

// --------------------------------------------------------------------------------------------------------------------
