#pragma once

#include "CoreMinimal.h"

#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AsErrorParser.h"

// --------------------------------------------------------------------------------------------------------------------

// Emergency stub synthesizer for AssetRegistry accessor drift (Rev 10 dispatcher strategy #3).
//
// Background: BusterBlock and its plugins generate AS-side accessor files like
// `Script/Generated/RawAssets.as` containing one function per discovered asset:
//
//     namespace assets
//     {
//         TSoftObjectPtr<USkeletalMesh> MALE_SKEL_NEW()
//             { return TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath("/Game/Raw/SKM/MALE_SKEL_NEW.MALE_SKEL_NEW")); }
//         ...
//     }
//     namespace assets::load { USkeletalMesh MALE_SKEL_NEW() { ... } }
//
// Drift class: when a teammate (or a teammate's automation) regenerates the
// file after adding a new asset but does NOT commit the regenerated `.as`,
// pulling their content forward leaves AS callers referencing
// `assets::MALE_SKEL_NEW()` while the committed `.as` lacks that function.
// AS compile fails with "No matching signatures to 'assets::MALE_SKEL_NEW()'"
// and the editor wedges on the Hazelight modal.
//
// Recovery: synthesize a minimum-viable accessor stub appended to the right
// `*Assets.as` file. AS merges multiple `namespace X { ... }` blocks, so the
// caller resolves to our stub on next compile and the editor unwedges. On
// `OnPostCompile`, the real generator (`UCkAssetRegistrySubsystem::Generate
// AllAssetRegistries`) overwrites the file and the stub vanishes.
//
// Two-tier strategy for resolving the accessor's UClass return type:
//
//   * **Tier 1 — non-BP assets** (USkeletalMesh, UTexture2D, UDataAsset
//     subclasses, etc.): `FAssetData::AssetClassPath` from a sync AR scan
//     gives the right class directly. No load needed. Fast (~microsecond).
//
//   * **Tier 2 — Blueprint assets**: `AssetClassPath` reports `UBlueprint`
//     (useless — the accessor needs the BP's *native parent*, not UBlueprint).
//     Sync-load the BP via `AssetData.GetAsset()` and walk
//     `LoadedBlueprint->ParentClass` via `Get_NonBlueprintParentClass`
//     (existing logic in `UCkAssetRegistrySubsystem`). Slow (~50-200ms per
//     asset due to the sync load).
//
//   * **Tier 3 — BP load fails** (native parent not yet loaded into UObject
//     space at modal-tick time, or AR scan missed the asset): REFUSED for
//     all flavors as of 2026-05-13 (revised from the original 2026-05-12
//     policy).
//
//     The original policy emitted a `TSoftObjectPtr<UObject>` stub for
//     SoftRef/SoftClass on the assumption that the caller's typed
//     assignment would produce a "follow-up AS error pointing at the right
//     line — better diagnostic than a wedge". Probe a2 (probe_a2.log,
//     2026-05-13) disproved that: the typed-conversion error
//     (`Cannot convert from TSoftObjectPtr<UObject> to TSoftObjectPtr<UWorld>`)
//     does NOT match either of FCkAsErrorParser's two recognized patterns.
//     Cycle 2 of the dispatcher parses zero actionable roots and the editor
//     wedges on the terminal banner instead of surfacing the original
//     `No matching signatures` error.
//
//     Refusing across the board (SoftRef + SoftClass + BlockingLoad) means
//     `Inject_AssetRegistryStub` returns `Success = false` with an
//     actionable manual-recovery banner in `ErrorMessage`, the dispatcher
//     logs that banner, and Hazelight's modal continues displaying the
//     original `No matching signatures` error which the user can act on.
//
// Determinism: same rules as the EntitySpawnParams synthesizer. Stub blocks
// carry a marker comment line (`Get_MarkerComment()`) so a forensic reader
// sees clearly that the block is recovery-injected.

namespace ck::angelscriptgenerator::self_heal
{
    // Outcome of an Inject_* call. Mirrors FCk_StubInjectionResult but adds
    // the resolved UClass name for forensic logging.
    struct CKANGELSCRIPTGENERATOR_API FCk_AssetStubInjectionResult
    {
        bool    Success = false;
        FString TargetFilePath;       // sibling stub file we wrote/appended to (empty on failure)
        FString InjectedBlock;        // verbatim text written into the stub (empty on failure)
        FString ResolvedAssetClass;   // e.g. "USkeletalMesh"; "UObject" for Tier 3 fallback
        FString ResolvedAssetPath;    // e.g. "/Game/Raw/SKM/MALE_SKEL_NEW.MALE_SKEL_NEW"
        FString ErrorMessage;         // populated on failure
        bool    UsedTier3Fallback = false; // dead as of 2026-05-13 (Tier 3 always refused); field retained for ABI stability of the result struct.
    };

    // Which accessor flavor an `assets[::*]::FOO()` error targets. The parser
    // captures the namespace + function name; we distill that into one of:
    //
    //   * SoftRef — `assets::FOO()` returning TSoftObjectPtr<X>
    //   * BlockingLoad — `assets::load::FOO()` returning X (or TSubclassOf<X>
    //                    for _Class variants on Blueprints)
    //   * SoftClass — `assets::FOO_Class()` returning TSoftClassPtr<X>
    //                 (Blueprint-only soft-class variant)
    //
    // Determined by inspecting `TargetNamespace` + `FunctionName` shape.
    enum class ECk_AssetAccessorFlavor : uint8
    {
        SoftRef,
        BlockingLoad,
        SoftClass,
    };

    // Generated `*Assets.as` file site descriptor — pair of (file path, the
    // UCkAssetRegistryConfig discovery root used to populate it). Parsed from
    // the header comment line each generated file emits.
    struct CKANGELSCRIPTGENERATOR_API FCk_AssetConfigSiteInfo
    {
        FString OutputPath;      // absolute path to the matched <Plugin>_Assets.as file
        FString DiscoveryRoot;   // package-style discovery root, e.g. "/Game/Raw/"
    };

    class CKANGELSCRIPTGENERATOR_API FCkAsAssetRegistryStubSynthesizer
    {
    public:
        // ---- Pure-logic surface (testable without engine state) -------------------

        // Decides which flavor an `assets[::*]::<func>` error targets.
        // `assets::FOO_Class()` -> SoftClass (only if FOO_Class ends with "_Class")
        // `assets::load::FOO()` -> BlockingLoad
        // `assets::FOO()`       -> SoftRef
        static auto
        Classify_AccessorFlavor(
            const FCk_AsParsedError& InError) -> ECk_AssetAccessorFlavor;

        // Strips the trailing "::load" from a namespace string. Used to
        // resolve the base namespace from which `UCkAssetRegistryConfig`
        // matching is done. Returns InNamespace unchanged if it doesn't end
        // with "::load".
        static auto
        Strip_LoadSuffix(
            const FString& InNamespace) -> FString;

        // Strips the trailing "_Class" from a function name. Used to find
        // the underlying asset whose `_Class` accessor was requested. Returns
        // InFunctionName unchanged if it doesn't end with "_Class".
        static auto
        Strip_ClassSuffix(
            const FString& InFunctionName) -> FString;

        // Builds a soft-ref accessor stub for the given accessor name and class:
        //   TSoftObjectPtr<UClass> FuncName() { return TSoftObjectPtr<UClass>(FSoftObjectPath("/Game/...")); }
        // Returns the function-body text (not a full namespace block).
        static auto
        Build_SoftRefAccessor(
            const FString& InFunctionName,
            const FString& InResolvedClassName,
            const FString& InAssetPath) -> FString;

        // Builds a soft-class accessor stub (Blueprint _Class variant):
        //   TSoftClassPtr<UClass> FuncName_Class() { return TSoftClassPtr<UClass>(FSoftObjectPath("/Game/..._C")); }
        // InAssetPath is the BASE asset path; this helper appends "_C".
        static auto
        Build_SoftClassAccessor(
            const FString& InFunctionName,        // includes "_Class" suffix
            const FString& InResolvedClassName,
            const FString& InAssetPath) -> FString;

        // Builds a blocking-load accessor stub:
        //   UClass FuncName()
        //   {
        //       if (ck::EnsureIfNot(...)) { return nullptr; }
        //       return System::LoadAsset_Blocking(<namespace>::FuncName());
        //   }
        static auto
        Build_BlockingLoadAccessor(
            const FString& InFunctionName,
            const FString& InResolvedClassName,
            const FString& InSoftNamespace) -> FString;

        // Wraps a function body in the appropriate namespace block + marker
        // comments. `InNamespace` is the full namespace path (e.g.
        // "assets", "assets::load"). Output ends with LINE_TERMINATOR.
        static auto
        Build_NamespaceBlock(
            const FString&           InNamespace,
            const FString&           InFunctionBody,
            const FCk_AsParsedError& InError) -> FString;

        // Marker comment string that prefixes every synthesized block. Same
        // shape as FCkAsStubSynthesizer::Get_MarkerComment but distinct text
        // so forensic readers can tell the two synthesizers apart.
        static auto
        Get_MarkerComment() -> FString;

        // Recovery-header banner written at the top of a freshly-created
        // sibling stub file. Public so tests can assert presence.
        static auto
        Get_StubFileHeader() -> FString;

        // Given a canonical generated `*Assets.as` path, returns the sibling
        // stub path (same directory, filename prefixed `_StubRecovery_`).
        // Public for tests + PostCompile cleanup.
        static auto
        Derive_StubSiblingPath(
            const FString& InCanonicalFilePath) -> FString;

        // Parses a single generated `*Assets.as` file's `// Source config: ...`
        // header line. Returns false (and leaves outputs unspecified) when the
        // header isn't present in the first ~10 lines of the file or the file
        // can't be read. OutSite.OutputPath is set to InFilePath on success;
        // OutNamespace receives the bracketed namespace token (e.g. "assets").
        static auto
        Try_ParseConfigSiteHeader(
            const FString&            InFilePath,
            FCk_AssetConfigSiteInfo&  OutSite,
            FString&                  OutNamespace) -> bool;

        // Scans the given directories non-recursively-by-pattern for files
        // matching `*Assets.as`, parses each one's header via
        // Try_ParseConfigSiteHeader, and returns those whose parsed namespace
        // equals InNamespace. Order follows IFileManager::FindFilesRecursive
        // (alphabetical) for determinism.
        static auto
        Collect_MatchingSites(
            const TArray<FString>& InDirs,
            const FString&         InNamespace) -> TArray<FCk_AssetConfigSiteInfo>;

        // Among candidates with the same matching namespace, picks the one
        // whose DiscoveryRoot is a prefix of the asset's package path. Returns
        // the index into InCandidates, or INDEX_NONE if no candidate's root
        // prefixes the asset path. Comparison is case-insensitive on the
        // package-path side; trailing '/' on DiscoveryRoot is normalized.
        //
        // When multiple candidates' roots all prefix the path (e.g. one is
        // "/Game/" and another is "/Game/Raw/"), the LONGEST match wins —
        // disambiguating nested discovery roots.
        static auto
        Pick_BestSite_ByAssetPath(
            const TArray<FCk_AssetConfigSiteInfo>& InCandidates,
            const FString&                         InAssetPackagePath) -> int32;

        // ---- Live entry point (requires engine state) -----------------------------

        // Top-level injector. Composes the helpers above:
        //   1. Classify the accessor flavor from the parsed error.
        //   2. Discover all UCkAssetRegistryConfig data assets (sync AR scan).
        //   3. Match the error's namespace against config.Namespace (stripping ::load).
        //   4. Find the asset in config.AssetDiscoveryRoot whose generated
        //      function name equals the error's FunctionName (or its
        //      _Class-stripped base).
        //   5. Resolve UClass via Tier 1 / Tier 2 / Tier 3.
        //   6. Build the stub block.
        //   7. UTF-16 LE atomic-append to config.OutputFileName (under the
        //      same resolved output directory the real generator uses).
        //   8. Touch caller mtime to nudge the hot-reload thread.
        //
        // Returns Success=false with a populated ErrorMessage when any step
        // fails (no matching config, asset not found, Tier 3 refusal for
        // blocking-load, file write failed, etc.). The caller (dispatcher)
        // logs the message and returns false to signal "strategy did not
        // progress recovery."
        static auto
        Inject_AssetRegistryStub(
            const FCk_AsParsedError& InError) -> FCk_AssetStubInjectionResult;
    };
}

// --------------------------------------------------------------------------------------------------------------------
