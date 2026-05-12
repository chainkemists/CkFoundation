#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

// Parses Hazelight's AngelScript compile-error output into typed root-cause records
// for the AS bootstrap self-heal dispatcher (Rev 10).
//
// Two error patterns are recognized — both correspond to deadlock classes that
// the dispatcher can act on:
//
//   * "No matching signatures to '<NS>::<func>(<args>)'"  — stale EntitySpawnParams
//                                                            stub OR missing asset
//                                                            registry accessor.
//   * "Identifier '<X>' is not a data type"               — missing dynamic-handle
//                                                            JSON entry.
//
// Anything else (cascade noise: "Unknown", "<X> is not declared", etc.) is dropped
// from the result. The dispatcher treats "compile failed but parser returned 0
// roots" as an unrecognized-failure case and surfaces a terminal banner.
//
// Error format is NOT a stable Hazelight public API contract. The unit tests
// under Tests/Test_AsErrorParser.cpp snapshot the three known-good formats
// (captured 2026-05-11 via the corruption probes). If those tests fail after an
// engine upgrade, update the parser before re-enabling the dispatcher.

namespace ck::angelscriptgenerator::self_heal
{
    enum class ECk_AsParsedError_Kind : uint8
    {
        NoMatchingSignatures,
        IdentifierNotADataType,
    };

    struct CKANGELSCRIPTGENERATOR_API FCk_AsParsedError
    {
        ECk_AsParsedError_Kind Kind = ECk_AsParsedError_Kind::NoMatchingSignatures;
        FString                FilePath;
        int32                  Line   = -1;
        int32                  Column = -1;

        // Populated when Kind == NoMatchingSignatures.
        FString TargetNamespace;   // e.g. "UBb_DeliveryTruck_EntityScript", "assets"
        FString FunctionName;      // e.g. "Params", "MALE_SKEL_NEW"
        FString ArgsList;          // e.g. "const FTransform"; empty for no-arg

        // Populated when Kind == IdentifierNotADataType.
        FString MissingIdentifier; // e.g. "FCk_Handle_CheckoutCounter"
        FString LookupScope;       // namespace name from "in namespace 'X' or parent";
                                   // empty when the error said "in global namespace"
                                   // or had no in-clause.

        auto operator==(const FCk_AsParsedError& Other) const -> bool;
        auto operator!=(const FCk_AsParsedError& Other) const -> bool { return NOT (*this == Other); }
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKANGELSCRIPTGENERATOR_API FCkAsErrorParser
    {
    public:
        // Parses a raw Hazelight AS compile-error block (typically the contents of
        // the InErrors array passed to the OnReloadHadErrors delegate, joined by
        // newlines, OR the equivalent log capture) into a flat list of recognized
        // root-cause errors. Preserves the order of appearance in the input.
        //
        // Lines that don't match a recognized pattern are silently dropped — the
        // dispatcher uses the result count to decide between "I can act" and
        // "show terminal banner".
        static auto
        ParseErrors(
            const FString& InRawErrorOutput) -> TArray<FCk_AsParsedError>;

        // Returns the unique set of root causes from InErrors, keyed by the
        // actionable content (qualified identifier + args, or missing identifier
        // name). Source file / line / column are NOT part of the dedup key — the
        // same root cause appears across many files in cascade, and the
        // dispatcher only needs one strategy invocation per unique root.
        //
        // Preserves order of first appearance.
        static auto
        DeduplicateRoots(
            const TArray<FCk_AsParsedError>& InErrors) -> TArray<FCk_AsParsedError>;
    };
}

// --------------------------------------------------------------------------------------------------------------------
