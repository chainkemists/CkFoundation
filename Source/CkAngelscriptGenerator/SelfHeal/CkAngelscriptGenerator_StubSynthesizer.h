#pragma once

#include "CoreMinimal.h"

#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AsErrorParser.h"

// --------------------------------------------------------------------------------------------------------------------

// Emergency stub synthesizer for the Rev 10 self-heal dispatcher.
//
// Given a parsed `No matching signatures to '<NS>::Params(<args>)'` error,
// synthesizes a minimum-viable USTRUCT + namespace block satisfying the
// missing accessor into a sibling `_StubRecovery_<Plugin>_EntitySpawnParams.as`
// file. AS namespace-merge across files makes compile succeed without
// touching the canonical. Sibling files are gitignored + deleted by
// PostCompile after a successful compile.
//
// Stub shape: empty struct + Params() returning a default-constructed value.
// Arg names are Arg0, Arg1, ... since the AS error gives types not names.
// Same-session repeat drifts for the same accessor are dedup-gated (the
// 2026-05-14 fix that prevented duplicate-function compile failures).
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
