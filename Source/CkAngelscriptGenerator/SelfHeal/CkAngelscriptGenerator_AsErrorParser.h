#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

// Parses Hazelight's AS compile-error output into typed root-cause records;
// cascade noise is dropped. The error format is NOT a stable Hazelight API:
// Tests/Test_AsErrorParser.cpp snapshots the known-good formats and is the
// engine-upgrade canary. Recognized patterns and the strategy each maps to:
// CkAngelscriptGenerator/CLAUDE.md.

namespace ck::angelscriptgenerator::self_heal
{
    enum class ECk_AsParsedError_Kind : uint8
    {
        NoMatchingSignatures,
        IdentifierNotADataType,
        AdjacentStringLiteral,         // "Instead found '<string constant>'" — `"foo " "bar"` C-style splice.
        BareCtorNoMatchingSignatures,  // `No matching signatures to '<Ident>(<args>)'` — no `::`.
    };

    struct CKANGELSCRIPTGENERATOR_API FCk_AsParsedError
    {
        ECk_AsParsedError_Kind Kind = ECk_AsParsedError_Kind::NoMatchingSignatures;
        FString                FilePath;
        int32                  Line   = -1;
        int32                  Column = -1;

        // Populated for Kind == NoMatchingSignatures.
        FString TargetNamespace;
        FString FunctionName;
        FString ArgsList;          // empty for no-arg; also carries the ctor args for BareCtorNoMatchingSignatures

        // Populated for Kind == IdentifierNotADataType (the missing type) and
        // BareCtorNoMatchingSignatures (the constructed type).
        FString MissingIdentifier;
        FString LookupScope;       // empty when "in global namespace" or absent

        auto operator==(const FCk_AsParsedError& Other) const -> bool;
        auto operator!=(const FCk_AsParsedError& Other) const -> bool { return NOT (*this == Other); }
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKANGELSCRIPTGENERATOR_API FCkAsErrorParser
    {
    public:
        // Unrecognized lines are silently dropped — the dispatcher uses the
        // returned count to decide "I can act" vs "terminal banner".
        static auto ParseErrors      (const FString& InRawErrorOutput)      -> TArray<FCk_AsParsedError>;

        // Dedup key is actionable content only (qualified identifier + args, or
        // missing identifier) — file/line/column are deliberately excluded, since
        // one root appears across many cascade sites and needs one strategy run.
        static auto DeduplicateRoots(const TArray<FCk_AsParsedError>& InErrors) -> TArray<FCk_AsParsedError>;
    };
}

// --------------------------------------------------------------------------------------------------------------------
