#pragma once

#include "CoreMinimal.h"

#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AsErrorParser.h"
#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AsSourceScanner.h"

// --------------------------------------------------------------------------------------------------------------------

// Emergency stub synthesizer for the self-heal dispatcher (Rev 10 dispatch
// model; Rev 11 adds the stale-canonical quarantine escalation —
// Quarantine_And_ResynthesizeFullShapes below).
//
// Synthesizes minimum-viable USTRUCT + namespace blocks satisfying missing
// EntitySpawnParams accessors into a sibling
// `_StubRecovery_<Plugin>_EntitySpawnParams.as` file. AS namespace-merge
// across files makes compile succeed without touching the canonical.
// Sibling files are gitignored + deleted by PostCompile after a successful
// compile.
//
// Two stub flavors:
//
//  * SOURCE-DERIVED FULL SHAPE (preferred) — the class's declaring .as file
//    is parsed (FCkAsSourceScanner) for its flattened ExposeOnSpawn
//    properties, and the stub mirrors the real generator's output: fielded
//    struct (real names + types, no defaults) + positional ctor + both
//    Params() overloads. Covers ALL caller patterns, including direct
//    construction and field access (`P.Phase = ...`) — the wholesale-missing
//    case a gitignored canonical creates on every fresh clone.
//
//  * ERROR-TEXT FALLBACK — built from the compile error alone: empty struct
//    + Params(<arg types>) returning a default-constructed value. Arg names
//    are Arg0, Arg1, ... since the AS error gives types not names. Covers
//    Params()-overload callers only; retained as the floor when the class
//    source can't be found/parsed, and as the per-signature path for
//    incremental drift (struct already present in the canonical).
//
// Same-session repeat drifts for the same accessor are dedup-gated on the
// CANONICAL (const-stripped) signature, with a declaration-level backstop
// (the 2026-05-14 fix, extended 2026-06 after bulk synthesis let
// const-aliased raw args duplicate the same emitted overload).
//
// See CkAngelscriptGenerator/Claude.md for the full sibling-file model.

namespace ck::angelscriptgenerator::self_heal
{
    // Typed failure reasons so the dispatcher can branch on WHY an injection
    // failed (escalate vs fall back) instead of pattern-matching ErrorMessage
    // strings. Rev 11: StructExistsInCanonical and SameArityAmbiguous route
    // to the canonical-quarantine escalation — both are wedge states the
    // per-signature error-text path cannot heal (pinned by the 2026-06-11
    // stale-canonical incident: a 25-param canonical vs 29-arg callers with
    // mixed static types at the StoreEntity position).
    enum class ECk_StubInjectFailReason : uint8
    {
        None,                           // success (or no failure recorded)
        NotApplicable,                  // wrong error kind / unrecognized name shape
        ScanFailed,                     // class source not found / not parseable
        AnchorFailed,                   // no canonical path could be resolved
        WriteFailed,                    // file IO failure
        StructExistsInCanonical,        // stale-but-present canonical owns the struct — quarantine escalation
        StructExistsInSibling,          // an earlier error-text stub owns the struct — per-signature path
        FullShapeOnDiskStillMismatched, // prior-process full shape didn't satisfy the caller — per-signature path
        SameArityAmbiguous,             // fallback/typed same-arity overload conflict — quarantine escalation
    };

    struct CKANGELSCRIPTGENERATOR_API FCk_StubInjectionResult
    {
        bool    Success = false;
        ECk_StubInjectFailReason FailReason = ECk_StubInjectFailReason::None;
        FString TargetFilePath;    // sibling stub file (empty on failure)
        FString CanonicalFilePath; // canonical the injection resolved against (when resolution got that far)
        FString InjectedBlock;     // verbatim stub text (empty on failure)
        FString ErrorMessage;      // populated on failure
    };

    class CKANGELSCRIPTGENERATOR_API FCkAsStubSynthesizer
    {
    public:
        // InEmitStruct=true emits the stub `struct F<X>_SpawnParams { }` before
        // the namespace. Set false when the struct already exists in the
        // target file (common case). Returns empty for non-NoMatchingSignatures.
        static auto Build_EntityScriptParamsStub(
            const FCk_AsParsedError& InError,
            bool                     InEmitStruct) -> FString;

        // Convention: U<X> -> F<X>_SpawnParams (e.g. `UBb_DeliveryTruck_Entity-
        // Script` -> `FBb_DeliveryTruck_EntityScript_SpawnParams`).
        static auto Derive_SpawnParamsStructName(const FString& InNamespaceName) -> FString;

        // Inverse: F<X>_SpawnParams -> U<X>. Empty when the input doesn't
        // match the generated-struct name shape — the dispatcher uses that
        // as the classification gate for direct-construction errors.
        static auto Derive_ClassNameFromStructName(const FString& InStructName) -> FString;

        // Picks the candidate file that references the failing namespace or
        // its struct. Empty when nothing matches (caller falls back to
        // Anchor_ByCallerAsPath for the brand-new-namespace case).
        static auto Find_TargetFile_ByContent(
            const FString&         InNamespaceName,
            const TArray<FString>& InCandidateFilePaths) -> FString;

        static auto Has_SpawnParamsStruct(
            const FString& InFileContents,
            const FString& InStructName) -> bool;

        // Atomic-write: <stub>.stubtmp -> rename. Returns TargetFilePath on the
        // SIBLING (never the canonical).
        static auto Inject_EntityScriptParamsStub(
            const FCk_AsParsedError& InError,
            const TArray<FString>&   InCandidateFilePaths) -> FCk_StubInjectionResult;

        // Canonicalizes an error-reported args list to value-typed form (strips
        // "const " / trailing "&" per token). The AS error reports the call
        // site's argument category, not the declared parameter — a literal vs
        // an lvalue at two call sites of the SAME function yields "const int"
        // vs "int&". Stub emission, the dedup end-marker, and the dispatcher's
        // convergence key all use this normalization so those variants collapse
        // to one stub instead of two mutually-ambiguous overloads.
        //
        // Compiler placeholders for uninferable arg types (`<null handle>` for
        // a bare nullptr) map to UObject — emitting the placeholder verbatim
        // makes the stub file unparseable, wedging every subsequent compile
        // (2026-06-10 incident). Inject_* additionally refuses to land a
        // fallback-bearing stub alongside a typed stub of the same arity (or
        // vice versa): the pair is mutually ambiguous at every call site.
        static auto Normalize_ArgsList(const FString& InArgsList) -> FString;

        // Source-derived full-shape stub text for a scanned class shape:
        // fielded struct (no defaults) + positional ctor + Params() /
        // Params(allFields), mirroring the real generator's Format_ClassBlock.
        // Ends with a class-level full-shape marker plus per-overload
        // end-markers (same prefix the error-text dedup gate scans), so later
        // error-text re-fires for either canonical overload no-op.
        static auto Build_EntityScriptParamsStub_FullShape(
            const FCk_AsClassShape&  InShape,
            const FCk_AsParsedError& InError) -> FString;

        // Source-derived injection. Scans for U<InClassName>'s declaring .as,
        // parses the flattened exposed-property shape, and appends a
        // full-shape stub to the sibling. FAILS (caller falls back to the
        // error-text path) when:
        //  * the class source can't be found/parsed (scan failure), or
        //  * the struct already exists in the canonical or in a sibling stub
        //    that isn't our own full shape — incremental drift is owned by
        //    the per-signature error-text path.
        // A re-fire for a class whose full shape is already in the sibling
        // no-ops with Success = true.
        static auto Inject_EntityScriptParamsStub_SourceDerived(
            const FString&           InClassName,
            const FCk_AsParsedError& InError,
            const TArray<FString>&   InCandidateFilePaths,
            const TArray<FString>&   InScanRoots) -> FCk_StubInjectionResult;

        // `// End synthesized full-shape stub for <ClassName>` — the
        // class-level dedup marker for source-derived stubs.
        static auto Get_FullShapeMarkerLine(const FString& InClassName) -> FString;

        // Scans canonical-file contents for `namespace U<X>` blocks whose
        // derived `F<X>_SpawnParams` struct is also defined in the same text.
        // Used by the quarantine escalation to enumerate every class the
        // stale canonical covered, so ONE retry compile converges instead of
        // burning a bootstrap cycle per "every other class just lost its
        // struct". Pure string scan — unit-testable.
        static auto Enumerate_EntityScriptNamespaces(
            const FString& InCanonicalContents) -> TArray<FString>;

        // Rev 11 stale-canonical escalation. A PRESENT canonical whose
        // accessor signatures no longer match the entity-script source (e.g.
        // an untracked leftover after the gitignore flip survives a pull)
        // cannot be healed by the per-signature error-text path: error-text
        // stubs are typed from ONE call site's static arg types, and mixed-
        // type callers at the same position (typesafe vs base handle) make
        // any single same-arity overload unsatisfiable for the rest — the
        // same-arity ambiguity gate then wedges (SameArityAmbiguous).
        //
        // This routine converts "stale" into the proven "missing" bootstrap:
        //  1. Builds the class union: InSeedClassNames ∪ namespaces in the
        //     canonical ∪ classes named by markers in the existing sibling.
        //  2. Deletes the sibling stub (it may hold the wedged per-call-site
        //     stub; rebuilt cleanly below).
        //  3. Quarantines the canonical — forensic copy to
        //     Saved/CkSelfHeal/Quarantine/<name>.stale_<UTC>, then DELETE
        //     (the canonical is rewritten on every successful compile; the
        //     startup stub-sweep retention rule keeps the rebuilt sibling
        //     alive across a force-quit while its canonical is missing).
        //  4. Source-derived full-shape synthesis for every class in the
        //     union (exact-typed named params satisfy ALL call sites —
        //     typesafe→base by-value conversion is implicit in AS). Classes
        //     whose source can't be scanned (deleted classes) are logged and
        //     skipped; they simply drop out of the regenerated canonical.
        //
        // Success requires every seed class to have synthesized; skipped
        // non-seed classes don't fail the escalation.
        static auto Quarantine_And_ResynthesizeFullShapes(
            const FString&           InCanonicalPath,
            const TArray<FString>&   InSeedClassNames,
            const FCk_AsParsedError& InError,
            const TArray<FString>&   InScanRoots) -> FCk_StubInjectionResult;

        // Clears the session-static "full shapes written this process" set —
        // lets tests simulate the next-process boundary (headless cook
        // retry) where a signature miss against an on-disk full shape defers
        // to the per-signature path instead of no-op'ing.
        static auto Reset_SessionState_ForTests() -> void;

        static auto Get_MarkerComment    () -> FString;
        static auto Get_StubFileHeader   () -> FString;
        static auto Derive_StubSiblingPath(const FString& InCanonicalFilePath) -> FString;

        // Fallback anchor when Find_TargetFile_ByContent returns empty (the
        // brand-new-namespace case where no existing file references the
        // class). Walks up from the caller's .as to the nearest *.uplugin or
        // *.uproject and returns the canonical EntitySpawnParams.as path
        // under that owner's Script/Generated/. The path may not exist on
        // disk; atomic-write creates the sibling regardless. Empty string
        // when no manifest ancestor is found (defensive — should never
        // happen for a real caller file).
        static auto Anchor_ByCallerAsPath(const FString& InCallerAsFilePath) -> FString;
    };
}

// --------------------------------------------------------------------------------------------------------------------
