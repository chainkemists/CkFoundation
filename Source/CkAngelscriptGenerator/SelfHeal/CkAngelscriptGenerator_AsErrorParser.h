#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

// Parses Hazelight's AS compile-error output into typed root-cause records.
// Four patterns recognized. The first three correspond to drift classes the
// dispatcher can auto-fix; the fourth is an author-side authoring bug the
// dispatcher only diagnoses (no auto-fix — modifying user source code is
// out of contract):
//   * `No matching signatures to '<NS>::<func>(<args>)'`  — stale EntitySpawn-
//     Params stub OR missing asset registry accessor (auto-fix).
//   * `No matching signatures to '<Ident>(<args>)'`       — bare constructor-
//     style call on an unregistered type. Observed when a caller directly
//     constructs a generated `F<X>_SpawnParams` whose canonical file is
//     missing (gitignored ESP / fresh clone) — the dispatcher routes the
//     `*_SpawnParams` shapes to ESP synthesis (auto-fix); anything else is
//     an authoring error left to the terminal banner.
//   * `Identifier '<X>' is not a data type`               — missing
//     dynamic-handle JSON entry, or a `F<X>_SpawnParams` used as a declared
//     type while its canonical file is missing (auto-fix).
//   * `Instead found '<string constant>'`                 — adjacent string
//     literals in author source (`"foo " "bar"` C-style splice). AS rejects;
//     the dispatcher recognizes the pattern but only surfaces a clear
//     "join into one literal" banner.
//
// Cascade noise ("Unknown", "<X> is not declared", etc.) is dropped. The
// dispatcher treats "compile failed but parser returned 0 roots" as an
// unrecognized-failure case and surfaces a terminal banner.
//
// Error format is NOT a stable Hazelight public API contract. The unit tests
// under Tests/Test_AsErrorParser.cpp snapshot the known-good formats
// (captured 2026-05-11 via the corruption probes; bare-ctor form derived
// from the 2026-06 fresh-clone ESP boot test) — engine-upgrade canary.
// If they go red, fix the parser regex against the new format before the
// dispatcher can resume working.

namespace ck::angelscriptgenerator::self_heal
{
    enum class ECk_AsParsedError_Kind : uint8
    {
        NoMatchingSignatures,
        IdentifierNotADataType,
        AdjacentStringLiteral,  // Hazelight emits "Expected ')' or ','" + "Instead found '<string constant>'" — `"foo " "bar"` C-style splice.
        BareCtorNoMatchingSignatures,  // `No matching signatures to '<Ident>(<args>)'` — no `::`; constructor-style call on an unregistered type.
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
        FString ArgsList;          // e.g. "const FTransform"; empty for no-arg. Also populated for BareCtorNoMatchingSignatures (the ctor args).

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
        // Lines that don't match a recognized pattern are silently dropped —
        // dispatcher uses the count to decide "I can act" vs "terminal banner".
        static auto ParseErrors      (const FString& InRawErrorOutput)      -> TArray<FCk_AsParsedError>;

        // Dedup key: actionable content (qualified identifier + args, or
        // missing identifier). Source file/line/column NOT in the key — the
        // same root appears across many cascade sites and the dispatcher
        // only needs one strategy invocation per unique root.
        static auto DeduplicateRoots(const TArray<FCk_AsParsedError>& InErrors) -> TArray<FCk_AsParsedError>;
    };
}

// --------------------------------------------------------------------------------------------------------------------
