#pragma once

#include "CoreMinimal.h"

#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AsErrorParser.h"

// --------------------------------------------------------------------------------------------------------------------

// AS bootstrap self-heal dispatcher (Rev 11 = Rev 10 error-driven dispatch
// + stale-canonical quarantine escalation). Bridges parsed AS-compile-error
// roots to recovery actions and defers their apply.
//
// Why deferred — empirical finding 2026-05-12: the hot-reload checker thread
// has not yet started when OnReloadHadErrors fires during initial-compile
// failure. It starts when Hazelight opens its AS-failure modal, AFTER our hook
// returns. Its first scan establishes mtime baselines for every .as file, so
// any file we write before that first scan is invisible — the baseline IS
// our new content, no future scan flags it as changed, and the modal never
// triggers a retry compile.
//   * Bootstrap: hook FSlateApplication::GetOnModalLoopTickEvent — fires
//     after the modal is open and the thread is running.
//   * Mid-session (Issue #7, 2026-05-13): hot-reload failures don't open a
//     modal, so use FTSTicker instead.

namespace ck::angelscriptgenerator::self_heal
{
    enum class ECk_RecoveryStrategy : uint8
    {
        SynthesizeStub_EntitySpawnParams,
        KickGenerator_DynamicHandle,
        KickGenerator_AssetRegistry,
        // A generated EntitySpawnParams canonical references a type that no
        // longer exists — e.g. an ExposeOnSpawn field enum/struct renamed or
        // unified away out from under the stale, machine-local canonical after
        // a pull. The compile error ('Identifier <X> is not a data type') is
        // located INSIDE the canonical, names no entity-script class, and
        // matches neither the FCk_Handle_ nor the F<X>_SpawnParams shape, so
        // the per-class SynthesizeStub path can't reach it. Quarantine the
        // whole canonical (forensic copy + delete — it is gitignored and
        // rebuilt from source) and bulk-resynthesize full shapes for every
        // class it covered. Keyed on the error's LOCATION (an ESP canonical
        // path), so the same "not a data type" error in author source stays a
        // terminal banner — that is a real authoring bug, not stale generated
        // output.
        Quarantine_StaleEspCanonical,
        // Author-side authoring bug — dispatcher recognizes the pattern and
        // emits an actionable banner, but does NOT mutate user source. Counted
        // as an "actionable" strategy so the dispatcher doesn't fall through
        // to the generic "no recognized roots" terminal (which leaves
        // headless test runs without an actionable diagnostic).
        Author_FixupRequired_AdjacentStringLiteral,
        Unrecognized,
    };

    struct CKANGELSCRIPTGENERATOR_API FCk_RecoveryAction
    {
        ECk_RecoveryStrategy Strategy = ECk_RecoveryStrategy::Unrecognized;
        FCk_AsParsedError    Error;
    };

    // One DynamicHandle sibling-stub entry, as written into
    // _StubRecovery_DynamicHandleTypes.json by Append_DynamicHandleStubEntries.
    struct CKANGELSCRIPTGENERATOR_API FCk_DynamicHandleStubEntry
    {
        FString TypeName;
        FString ShortName;
    };

    class CKANGELSCRIPTGENERATOR_API FCkAsRecoveryDispatcher
    {
    public:
        // Bootstrap-only. Cold-start runaway would wedge the editor behind
        // the AS modal indefinitely; hard-stop after MaxCycles. Mid-session
        // bypasses the cap (user can intervene) and resets it at
        // Mark_BootstrapComplete.
        static constexpr int32 MaxCycles = 3;

        // Per-signature mid-session convergence cap. Each time a stub-synthesis
        // strategy is dispatched for the same <NS>::<func>(<args>) (or
        // DynamicHandle::<TypeName>) key, a counter increments. On the Nth
        // attempt the dispatcher refuses synthesis, blacklists the key for the
        // rest of the session, and emits Log_TerminalBanner_ConvergenceFailed.
        // Catches "dueling-overloads" loops where the upstream cause (caller
        // arg order mismatch, literal-vs-lvalue mutability conflict, type
        // drift) is outside what self-heal can repair. Symmetric with
        // MaxCycles to keep the boot-time and mid-session caps predictable.
        static constexpr int32 MaxPerSignatureRepeats = 3;

        static auto Classify       (const FCk_AsParsedError& InError) -> ECk_RecoveryStrategy;
        static auto BuildActionPlan(const TArray<FCk_AsParsedError>& InDedupedRoots) -> TArray<FCk_RecoveryAction>;

        // Hook handler for FAngelscriptCodeModule::GetReloadHadErrors. Parses,
        // classifies, queues, and subscribes to modal-tick (bootstrap) or
        // FTSTicker (mid-session). Terminal banner on cycle-cap / zero roots /
        // all-actions-failed.
        static auto OnAngelscriptReloadHadErrors() -> void;

        static auto Get_CyclesRun  () -> int32;
        static auto Reset_CyclesRun() -> void;

        // Invoked from the Module's PostCompile hook (fires on SUCCESSFUL
        // compile only). Drops queued-but-undrained recovery actions and
        // disarms the modal-tick / FTSTicker drains: a deferred cycle firing
        // after a clean compile would re-synthesize stubs from stale error
        // records and re-corrupt a healthy state (observed 2026-06-10 — the
        // re-corruption fired minutes after the user had hand-fixed the
        // wedge). The per-signature convergence trackers and blacklist
        // intentionally survive — they exist precisely to span cleanup boundaries.
        static auto Clear_PendingRecoveryState() -> void;

        // Routes OnAngelscriptReloadHadErrors to modal-tick vs FTSTicker.
        // Flipped by the Module's OnFEngineLoopInitComplete lambda.
        static auto Is_BootstrapMode     () -> bool;
        static auto Mark_BootstrapComplete() -> void;

        // DynamicHandle: synthesized JSON stub has empty RequiredFragments,
        // which produces a PERMISSIVE validator until OnPostEngineInit regen
        // + UpdateExistingDynamicHandle upgrade it to strict. Sub-second
        // window. Module reads this to decide whether to call
        // GenerateHandleTypeRegistry deferred.
        static auto Did_SynthesizeJsonStub_ThisSession() -> bool;
        static auto Mark_JsonStubSynthesized          () -> void;

        // AssetRegistry: even Tier 1/2 success may have landed the stub in
        // the wrong file (file-scan picks first match before asset-on-disk
        // lookup picks the owner). Module reads this to decide whether to
        // call GenerateAllAssetRegistries deferred for reshuffle.
        static auto Did_SynthesizeAssetRegistryStub_ThisSession() -> bool;
        static auto Mark_AssetRegistryStubSynthesized          () -> void;

        // Sibling stub path for the DynamicHandle registry: same dir as the
        // canonical, `_StubRecovery_` filename prefix. Shared by the heal path
        // and the boot-time pre-seed (FCkAsDhPreSeed).
        static auto Derive_DynamicHandleStubPath(const FString& InCanonicalJsonPath) -> FString;

        // Shared DynamicHandle sibling-stub entry writer — the SINGLE owner of
        // the entry shape (TypeName/ShortName/Description/SourceAsset/empty
        // RequiredFragments) so the heal path and the boot pre-seed cannot
        // drift. Appends entries whose TypeName isn't already in the sibling
        // at InStubJsonPath (file created when missing); atomic temp+move
        // write; when nothing appends, no file is written. Returns the number
        // appended; unset on parse/serialize/write failure. Callers own the
        // ownership gate (G9 drains / G12 pre-seed) and any in-memory
        // registry refresh.
        static auto Append_DynamicHandleStubEntries(
            const FString& InStubJsonPath,
            TArrayView<const FCk_DynamicHandleStubEntry> InEntries) -> TOptional<int32>;
    };
}

// --------------------------------------------------------------------------------------------------------------------
