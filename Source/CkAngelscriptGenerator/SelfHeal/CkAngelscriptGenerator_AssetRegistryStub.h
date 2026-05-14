#pragma once

#include "CoreMinimal.h"

#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AsErrorParser.h"

// --------------------------------------------------------------------------------------------------------------------

// Emergency stub synthesizer for AssetRegistry accessor drift (Rev 10 strategy #3).
//
// Background: BB and its plugins generate AS-side accessor files like
// `Script/Generated/RawAssets.as` containing one function per discovered asset
// (`TSoftObjectPtr<USkeletalMesh> MALE_SKEL_NEW() { ... }` etc.). When a
// teammate's regen of those files doesn't get committed alongside the AS
// callers that reference them, AS compile fails with
// `No matching signatures to 'assets::MALE_SKEL_NEW()'` and the editor wedges
// on the Hazelight modal.
//
// Recovery (sibling-file model): synthesize a minimum-viable accessor stub
// into `Script/Generated/_StubRecovery_<MatchedAssetsFile>.as` — a sibling
// file alongside the canonical, gitignored, deleted by the PostCompile hook
// after a successful compile. AS's multi-file namespace-merge resolves the
// missing accessor at compile time. Canonical files are never touched.
//
// Class resolution for the accessor's return type:
//   * Tier 1 — non-BP assets (USkeletalMesh, UTexture2D, UDataAsset etc.):
//     LoadObject + class walk via Get_NonBlueprintParentClass. Fast.
//   * Tier 2 — Blueprint assets: same LoadObject path, walks
//     LoadedBlueprint->ParentClass to find the native parent.
//   * Tier 3 — LoadObject fails (native parent module not yet loaded at
//     modal-tick, asset not on disk, etc.): REFUSED for all flavors as of
//     2026-05-13 (probe_a2.log).
//
//     The original 2026-05-12 policy emitted `TSoftObjectPtr<UObject>` stubs
//     on the assumption that the caller's typed-conversion error would point
//     at the right line. Probe a2 disproved that: the typed-conversion error
//     (`Cannot convert from TSoftObjectPtr<UObject> to TSoftObjectPtr<UWorld>`)
//     doesn't match either of FCkAsErrorParser's two recognized patterns.
//     Cycle 2 parses zero actionable roots and the editor wedges on the
//     terminal banner instead of surfacing the original `No matching
//     signatures` error.
//
//     Refusing across the board means `Inject_AssetRegistryStub` returns
//     `Success = false` with an actionable manual-recovery banner in
//     `ErrorMessage`, the dispatcher logs that banner, and Hazelight's modal
//     keeps showing the original `No matching signatures` error which the
//     user can act on.
//
// Discovery is AR-free — at modal-tick during initial-compile-failure, AR's
// `SearchAllAssets`/`GetAssetsByClass` paths exit early on
// `IsEngineStartupModuleLoadingComplete() == FALSE`. We file-scan
// `Script/Generated/*Assets.as` for `// Discovery root:` header lines instead.

namespace ck::angelscriptgenerator::self_heal
{
    // Outcome of an Inject_* call.
    struct CKANGELSCRIPTGENERATOR_API FCk_AssetStubInjectionResult
    {
        bool    Success = false;
        FString TargetFilePath;       // sibling stub file we wrote/appended to (empty on failure)
        FString InjectedBlock;        // verbatim text written into the stub (empty on failure)
        FString ResolvedAssetClass;   // e.g. "USkeletalMesh" (empty on failure)
        FString ResolvedAssetPath;    // e.g. "/Game/Raw/SKM/MALE_SKEL_NEW.MALE_SKEL_NEW" (empty on failure)
        FString ErrorMessage;         // populated on failure
        bool    UsedTier3Fallback = false; // dead as of 2026-05-13 (Tier 3 always refused); field retained for source compatibility
    };

    // Which accessor flavor an `assets[::*]::FOO()` error targets:
    //   * SoftRef       — `assets::FOO()` returning TSoftObjectPtr<X>
    //   * BlockingLoad  — `assets::load::FOO()` returning X (or TSubclassOf<X>
    //                     for _Class variants on Blueprints)
    //   * SoftClass     — `assets::FOO_Class()` returning TSoftClassPtr<X>
    //                     (Blueprint-only soft-class variant)
    enum class ECk_AssetAccessorFlavor : uint8
    {
        SoftRef,
        BlockingLoad,
        SoftClass,
    };

    // Generated `*Assets.as` file site descriptor — (file path, discovery
    // root) pair parsed from the file's `// Discovery root:` header line.
    struct CKANGELSCRIPTGENERATOR_API FCk_AssetConfigSiteInfo
    {
        FString OutputPath;      // absolute path to the matched <Plugin>_Assets.as file
        FString DiscoveryRoot;   // package-style discovery root, e.g. "/Game/Raw/"
    };

    class CKANGELSCRIPTGENERATOR_API FCkAsAssetRegistryStubSynthesizer
    {
    public:
        // ---- Pure-logic surface (testable without engine state) -------------------

        // `assets::FOO_Class()` -> SoftClass (only if FOO_Class ends with "_Class")
        // `assets::load::FOO()` -> BlockingLoad
        // `assets::FOO()`       -> SoftRef
        static auto
        Classify_AccessorFlavor(
            const FCk_AsParsedError& InError) -> ECk_AssetAccessorFlavor;

        // Strips trailing "::load" from a namespace string. Used to resolve
        // the base namespace for matching against `Discovery root:` headers.
        static auto
        Strip_LoadSuffix(
            const FString& InNamespace) -> FString;

        // Strips trailing "_Class" from a function name. Used to find the
        // underlying asset whose `_Class` accessor was requested.
        static auto
        Strip_ClassSuffix(
            const FString& InFunctionName) -> FString;

        // Soft-ref accessor body: `TSoftObjectPtr<UClass> FuncName() { ... }`.
        static auto
        Build_SoftRefAccessor(
            const FString& InFunctionName,
            const FString& InResolvedClassName,
            const FString& InAssetPath) -> FString;

        // Soft-class accessor body (BP _Class variant). InAssetPath is the
        // base asset path; this helper appends `_C` per the AS convention.
        static auto
        Build_SoftClassAccessor(
            const FString& InFunctionName,        // includes "_Class" suffix
            const FString& InResolvedClassName,
            const FString& InAssetPath) -> FString;

        // Blocking-load accessor body: ensure-guarded LoadAsset_Blocking that
        // delegates to the soft-ref accessor.
        static auto
        Build_BlockingLoadAccessor(
            const FString& InFunctionName,
            const FString& InResolvedClassName,
            const FString& InSoftNamespace) -> FString;

        // Wraps a function body in the appropriate namespace block + marker
        // comments. `InNamespace` is the full namespace path (e.g. "assets",
        // "assets::load"). Output ends with LINE_TERMINATOR.
        static auto
        Build_NamespaceBlock(
            const FString&           InNamespace,
            const FString&           InFunctionBody,
            const FCk_AsParsedError& InError) -> FString;

        // Marker prefixing every synthesized block. Same shape as
        // FCkAsStubSynthesizer::Get_MarkerComment but distinct text so
        // forensic readers can tell the two synthesizers apart.
        static auto
        Get_MarkerComment() -> FString;

        // Banner written at the top of a freshly-created sibling stub file.
        // Public for tests + PostCompile cleanup.
        static auto
        Get_StubFileHeader() -> FString;

        // Canonical path -> sibling stub path (`_StubRecovery_<filename>`
        // in the same directory). Public for tests + PostCompile cleanup.
        static auto
        Derive_StubSiblingPath(
            const FString& InCanonicalFilePath) -> FString;

        // Parses a single `*Assets.as` header line set. Returns false (and
        // leaves outputs unspecified) when the file can't be read or the
        // header isn't present in the first ~10 lines. Reads `// Discovery
        // root:` for the root (canonical source — the `// Source config:`
        // line was historically malformed by a U+2192 arrow concatenation
        // bug before commit c408ee8be).
        static auto
        Try_ParseConfigSiteHeader(
            const FString&            InFilePath,
            FCk_AssetConfigSiteInfo&  OutSite,
            FString&                  OutNamespace) -> bool;

        // Scans InDirs for `*Assets.as` files whose `Try_ParseConfigSiteHeader`-
        // extracted namespace matches InNamespace. Alphabetical order from
        // FindFilesRecursive for determinism.
        static auto
        Collect_MatchingSites(
            const TArray<FString>& InDirs,
            const FString&         InNamespace) -> TArray<FCk_AssetConfigSiteInfo>;

        // Among same-namespace candidates, picks the one whose DiscoveryRoot
        // is a prefix of the asset's package path. Longest match wins (so
        // "/Game/Raw/" beats "/Game/" for an asset under "/Game/Raw/SKM/").
        // Returns INDEX_NONE when no candidate prefixes the asset path.
        static auto
        Pick_BestSite_ByAssetPath(
            const TArray<FCk_AssetConfigSiteInfo>& InCandidates,
            const FString&                         InAssetPackagePath) -> int32;

        // ---- Live entry point (requires engine state) -----------------------------

        // Top-level injector. Steps:
        //   1. Classify the accessor flavor from the parsed error.
        //   2. Collect_MatchingSites for the error's namespace
        //      (file-scan of Script/Generated/*Assets.as; no AR).
        //   3. For each candidate, try to locate the failing function's
        //      asset on disk under its DiscoveryRoot. The candidate whose
        //      root owns the asset is the target site.
        //   4. Resolve UClass via LoadObject + Get_NonBlueprintParentClass.
        //   5. Build the stub block via Build_*Accessor + Build_NamespaceBlock.
        //   6. Write/append the stub block to the sibling stub file
        //      (Derive_StubSiblingPath + atomic-write).
        //   7. Touch the caller's .as mtime to nudge the hot-reload thread.
        //
        // Returns Success=false with a populated ErrorMessage on any failure
        // path (no matching site, asset not found, LoadObject failed, etc.).
        // Tier 3 (UObject fallback) is REFUSED — see the file-header policy
        // rationale. The caller (dispatcher) logs the message and returns
        // false; Hazelight's modal continues displaying the original AS
        // error which the user can act on.
        static auto
        Inject_AssetRegistryStub(
            const FCk_AsParsedError& InError) -> FCk_AssetStubInjectionResult;
    };
}

// --------------------------------------------------------------------------------------------------------------------
