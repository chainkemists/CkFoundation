#pragma once

#include "CoreMinimal.h"

#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AsErrorParser.h"

// --------------------------------------------------------------------------------------------------------------------

// Emergency stub synthesizer for the Rev 10 self-heal dispatcher.
//
// Given a parsed "No matching signatures to '<NS>::Params(<args>)'" error from
// the EntitySpawnParams shape, synthesizes the minimum AS source that satisfies
// the missing accessor — a (possibly empty) USTRUCT plus a namespace block with
// the failing Params(...) overload — and injects it into the appropriate
// `Script/Generated/<Plugin>_EntitySpawnParams.as` file.
//
// The synthesized stub is intentionally minimum-viable:
//   * Struct has no fields. Required only so the namespace's Params() body has
//     a default-constructible return type.
//   * Params(<args>) body returns a default-constructed struct. Argument names
//     are auto-generated (Arg0, Arg1, ...) since the AS error gives us types
//     but not parameter names.
//   * No replacement of the existing (mangled / partial / outright missing)
//     namespace block in the file — AS allows multiple `namespace X { ... }`
//     blocks with the same name, so we just append. The wrong block becomes
//     dead code referenced by nothing.
//
// Why the stub is safe to ship:
//   * Real generator (FCkAngelscriptEntityScriptParamsGenerator::GenerateAll)
//     rewrites the entire file once the editor finishes booting, so the stub
//     is short-lived and overwritten without trace.
//   * Stubs are tagged with marker comments so a forensic reader sees clearly
//     that the block is recovery-injected, not authored.
//   * The file is NOT written with an updated "// source-hash:" line — if the
//     editor is force-quit while a stub is on disk, the next launch's drift
//     check will see the file as dirty relative to its committed/regenerated
//     state and force a clean regen. (CTO Rev 10 pushback #6.)

namespace ck::angelscriptgenerator::self_heal
{
    // Result of an Inject_* call.
    struct CKANGELSCRIPTGENERATOR_API FCk_StubInjectionResult
    {
        bool    Success = false;
        FString TargetFilePath;   // file path we appended to (empty on failure)
        FString InjectedBlock;     // verbatim text appended (empty on failure)
        FString ErrorMessage;      // populated on failure
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
        //   1. Find_TargetFile_ByContent across InCandidateFiles
        //   2. Read file, decide Has_SpawnParamsStruct
        //   3. Build_EntityScriptParamsStub with appropriate InEmitStruct
        //   4. Append to file (no source-hash update — see header docstring)
        //
        // Atomic-write semantics: writes to TargetFilePath + ".stubtmp", then
        // moves into place. If any step fails, the original file is unchanged.
        static auto
        Inject_EntityScriptParamsStub(
            const FCk_AsParsedError& InError,
            const TArray<FString>&   InCandidateFilePaths) -> FCk_StubInjectionResult;

        // Marker comment string that prefixes every synthesized block. Public
        // so tests and forensic tooling can detect injected stubs.
        static auto
        Get_MarkerComment() -> FString;
    };
}

// --------------------------------------------------------------------------------------------------------------------
