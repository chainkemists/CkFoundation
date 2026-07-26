#pragma once

#include "CoreMinimal.h"

#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AsErrorParser.h"
#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AsSourceScanner.h"

// --------------------------------------------------------------------------------------------------------------------

// Emergency stub synthesizer for the self-heal dispatcher. Stub model, flavors,
// and dedup rules: CkAngelscriptGenerator/CLAUDE.md § Sibling-file stub model.

namespace ck::angelscriptgenerator::self_heal
{
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
        // InEmitStruct=false when the struct already exists in the target file
        // (common case). Returns empty for non-NoMatchingSignatures errors.
        static auto Build_EntityScriptParamsStub(
            const FCk_AsParsedError& InError,
            bool                     InEmitStruct) -> FString;

        // Convention: U<X> -> F<X>_SpawnParams.
        static auto Derive_SpawnParamsStructName(const FString& InNamespaceName) -> FString;

        // Inverse: F<X>_SpawnParams -> U<X>. Empty when the name shape doesn't
        // match — the dispatcher's classification gate for direct construction.
        static auto Derive_ClassNameFromStructName(const FString& InStructName) -> FString;

        // Empty when nothing matches — caller falls back to Anchor_ByCallerAsPath.
        static auto Find_TargetFile_ByContent(
            const FString&         InNamespaceName,
            const TArray<FString>& InCandidateFilePaths) -> FString;

        static auto Has_SpawnParamsStruct(
            const FString& InFileContents,
            const FString& InStructName) -> bool;

        // Returns TargetFilePath on the SIBLING — the canonical is never written.
        static auto Inject_EntityScriptParamsStub(
            const FCk_AsParsedError& InError,
            const TArray<FString>&   InCandidateFilePaths) -> FCk_StubInjectionResult;

        // Value-typed canonical form: strips "const " / trailing "&" per token, and
        // maps uninferable placeholders (`<null handle>`) to UObject. Stub emission,
        // the dedup end-marker and the convergence key all key off this form.
        static auto Normalize_ArgsList(const FString& InArgsList) -> FString;

        // Ends with a class-level full-shape marker plus per-overload end-markers
        // carrying the same prefix the error-text dedup gate scans.
        static auto Build_EntityScriptParamsStub_FullShape(
            const FCk_AsClassShape&  InShape,
            const FCk_AsParsedError& InError) -> FString;

        // FAILS (caller falls back to the error-text path) when the class source
        // can't be scanned, or when the struct already exists in the canonical or
        // in a sibling stub that isn't our own full shape. InCache (drain-scoped)
        // reuses one *.as enumeration + contents cache across classes.
        static auto Inject_EntityScriptParamsStub_SourceDerived(
            const FString&           InClassName,
            const FCk_AsParsedError& InError,
            const TArray<FString>&   InCandidateFilePaths,
            const TArray<FString>&   InScanRoots,
            FCk_AsSourceScanCache*   InCache = nullptr) -> FCk_StubInjectionResult;

        // `// End synthesized full-shape stub for <ClassName>` — the class-level
        // dedup marker for source-derived stubs.
        static auto Get_FullShapeMarkerLine(const FString& InClassName) -> FString;

        // `namespace U<X>` blocks whose derived `F<X>_SpawnParams` struct is defined
        // in the same text. Pure string scan — unit-testable.
        static auto Enumerate_EntityScriptNamespaces(
            const FString& InCanonicalContents) -> TArray<FString>;

        // Stale-canonical escalation (CkAngelscriptGenerator/CLAUDE.md § Recovery
        // strategies). Success requires every seed class to have synthesized;
        // skipped non-seed classes don't fail the escalation.
        static auto Quarantine_And_ResynthesizeFullShapes(
            const FString&           InCanonicalPath,
            const TArray<FString>&   InSeedClassNames,
            const FCk_AsParsedError& InError,
            const TArray<FString>&   InScanRoots,
            FCk_AsSourceScanCache*   InCache = nullptr) -> FCk_StubInjectionResult;

        // Clears the session-static "full shapes written this process" set, letting
        // tests simulate the next-process boundary (headless cook retry).
        static auto Reset_SessionState_ForTests() -> void;

        static auto Get_MarkerComment    () -> FString;
        static auto Get_StubFileHeader   () -> FString;
        static auto Derive_StubSiblingPath(const FString& InCanonicalFilePath) -> FString;

        // Walks up from the caller's .as to the nearest *.uplugin / *.uproject and
        // returns that owner's Script/Generated/<Owner>_EntitySpawnParams.as. The
        // path need not exist on disk; empty when no manifest ancestor is found.
        static auto Anchor_ByCallerAsPath(const FString& InCallerAsFilePath) -> FString;
    };
}

// --------------------------------------------------------------------------------------------------------------------
