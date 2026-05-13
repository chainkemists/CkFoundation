#pragma once

#include "CoreMinimal.h"

#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AsErrorParser.h"

// --------------------------------------------------------------------------------------------------------------------

// Emergency stub synthesizer for the Rev 10 self-heal dispatcher.
//
// Given a parsed "No matching signatures to '<NS>::Params(<args>)'" error from
// the EntitySpawnParams shape, synthesizes the minimum AS source that satisfies
// the missing accessor — a (possibly empty) USTRUCT plus a namespace block with
// the failing Params(...) overload — and writes it to a SIBLING file alongside
// the canonical `Script/Generated/<Plugin>_EntitySpawnParams.as`. The sibling
// is named `_StubRecovery_<Plugin>_EntitySpawnParams.as`.
//
// Why a sibling file (not in-place append):
//   * Canonical generated files stay byte-clean from HEAD — no marker-scan
//     defense layer required, and `git add -A && commit` cannot accidentally
//     stage a stub-mutated canonical.
//   * AS merges multiple `namespace X { ... }` blocks across files — the
//     sibling contributes its accessor to the merged scope and compile
//     succeeds without touching the canonical.
//   * `.gitignore` covers `_StubRecovery_*` patterns so stub files physically
//     cannot be staged.
//   * The PostCompile hook deletes stub files after a successful AS compile;
//     no marker scan needed and force-quit recoveries are trivial (next
//     launch's AS compile succeeds because the stub file is still there, then
//     PostCompile cleans it up).
//
// The synthesized stub is intentionally minimum-viable:
//   * Struct has no fields. Required only so the namespace's Params() body has
//     a default-constructible return type.
//   * Params(<args>) body returns a default-constructed struct. Argument names
//     are auto-generated (Arg0, Arg1, ...) since the AS error gives us types
//     but not parameter names.
//   * Multiple drifts in the same session accumulate into the same stub file
//     (the synthesizer appends to it if it already exists).

namespace ck::angelscriptgenerator::self_heal
{
    // Result of an Inject_* call.
    struct CKANGELSCRIPTGENERATOR_API FCk_StubInjectionResult
    {
        bool    Success = false;
        FString TargetFilePath;   // sibling stub file we wrote/appended to (empty on failure)
        FString InjectedBlock;    // verbatim text written into the stub (empty on failure)
        FString ErrorMessage;     // populated on failure
    };

    class CKANGELSCRIPTGENERATOR_API FCkAsStubSynthesizer
    {
    public:
        // Pure-string stub builder. Produces the AS source text that, when
        // appended to an EntitySpawnParams.as file, satisfies the missing
        // accessor described by InError.
        //
        // If InEmitStruct is true, the output includes a stub `struct
        // F<X>_SpawnParams { }` definition before the namespace. Set to false
        // when the caller has already confirmed the struct exists in the
        // target file (the common case for namespace-only corruption).
        //
        // Returns empty string if InError.Kind != NoMatchingSignatures, since
        // this synthesizer only handles that error class.
        static auto
        Build_EntityScriptParamsStub(
            const FCk_AsParsedError& InError,
            bool                     InEmitStruct) -> FString;

        // Derives the struct name from the namespace name following the real
        // generator's convention: strip leading 'U' from the class identifier
        // and prepend 'F', then append '_SpawnParams'.
        //   "UBb_DeliveryTruck_EntityScript" -> "FBb_DeliveryTruck_EntityScript_SpawnParams"
        // Returns empty string for an empty / malformed input.
        static auto
        Derive_SpawnParamsStructName(
            const FString& InNamespaceName) -> FString;

        // Scans the supplied candidate file paths for one that already contains
        // a reference to either the namespace name or its derived struct name.
        // Used by Inject_* to pick which `<Plugin>_EntitySpawnParams.as` file
        // to append the stub to.
        //
        // Returns:
        //   * the matching file path if exactly one candidate references the
        //     identifier (corrupted or intact).
        //   * empty string if none match — caller falls back to a default.
        static auto
        Find_TargetFile_ByContent(
            const FString&         InNamespaceName,
            const TArray<FString>& InCandidateFilePaths) -> FString;

        // Returns true if the target file already contains a `struct
        // F<X>_SpawnParams` definition (any variant — corrupted, valid, or
        // partial). Used by Inject_* to decide whether to set the
        // InEmitStruct flag on Build_*.
        static auto
        Has_SpawnParamsStruct(
            const FString& InFileContents,
            const FString& InStructName) -> bool;

        // Top-level injector. Composes the helpers above:
        //   1. Find_TargetFile_ByContent across InCandidateFiles to determine
        //      which plugin owns the affected entity-script class.
        //   2. Compute the sibling stub path (`_StubRecovery_<canonical>.as`
        //      in the same directory).
        //   3. Build_EntityScriptParamsStub (always emit struct for the first
        //      stub in the file; subsequent appends decide via the in-progress
        //      stub-file contents).
        //   4. If the stub file already exists, read+append. Otherwise write
        //      a fresh file with the recovery-header banner.
        //
        // Atomic-write semantics: writes to <stub>.stubtmp, then moves into
        // place. If any step fails, prior stub-file state is unchanged.
        //
        // The TargetFilePath returned in the result points at the SIBLING
        // stub file, NOT the canonical file.
        static auto
        Inject_EntityScriptParamsStub(
            const FCk_AsParsedError& InError,
            const TArray<FString>&   InCandidateFilePaths) -> FCk_StubInjectionResult;

        // Marker comment string that prefixes every synthesized block. Public
        // so tests and forensic tooling can detect injected stubs.
        static auto
        Get_MarkerComment() -> FString;

        // Returns the recovery-header banner text written at the top of a
        // freshly-created stub file. Public so tests can assert its presence.
        static auto
        Get_StubFileHeader() -> FString;

        // Given a canonical generated file path, returns the sibling stub
        // file path (same directory, filename prefixed with
        // `_StubRecovery_`). Public for tests + the PostCompile cleanup.
        static auto
        Derive_StubSiblingPath(
            const FString& InCanonicalFilePath) -> FString;
    };
}

// --------------------------------------------------------------------------------------------------------------------
