#pragma once

#include "CoreMinimal.h"

#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AsErrorParser.h"
#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AsSourceScanner.h"

// --------------------------------------------------------------------------------------------------------------------

// Emergency stub synthesizer for the Rev 10 self-heal dispatcher.
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
    struct CKANGELSCRIPTGENERATOR_API FCk_StubInjectionResult
    {
        bool    Success = false;
        FString TargetFilePath;   // sibling stub file (empty on failure)
        FString InjectedBlock;    // verbatim stub text (empty on failure)
        FString ErrorMessage;     // populated on failure
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
