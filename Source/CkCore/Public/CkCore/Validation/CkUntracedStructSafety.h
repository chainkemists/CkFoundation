#pragma once

#include "CoreMinimal.h"

class UScriptStruct;

namespace ck
{
    enum class ECk_UntracedStructSafety : uint8
    {
        GcIndependent,
        RequiresGcTracing,
        UnprovenOpaque,
    };

    struct CKCORE_API FCk_UntracedStructSafetyResult
    {
        ECk_UntracedStructSafety Safety = ECk_UntracedStructSafety::UnprovenOpaque;
        FString FailurePath;
        FString FailureReason;

        auto IsGcIndependent() const -> bool
        { return Safety == ECk_UntracedStructSafety::GcIndependent; }
    };

    // True for any struct DECLARED in AngelScript, whatever its size or field count: script structs reflect as
    // UASStruct, matched here by reflected class path so foundational CkCore need not link the optional
    // AngelscriptCode module. This is the enumeration predicate for reflection-walking fences over script types —
    // note it does NOT prove zero storage (that extra clause is what IsAngelScriptStruct adds internally for the
    // GC walk's field-less-struct branch, and a fence that reuses it would silently see only tag structs).
    CKCORE_API auto
    Is_AngelScriptDeclaredStruct(
        const UScriptStruct* InStruct) -> bool;

    // A UClass is collectable in principle - a Blueprint-generated class goes away on reload - so untraced storage
    // cannot hold one safely. A caller whose concern is narrower may say so explicitly.
    enum class ECk_UntracedStructSafety_ClassRefs : uint8
    {
        Reject,
        Accept,
    };

    // A struct owning references through AddStructReferencedObjects is unrooted in untraced storage, because nothing
    // there invokes its collector. TreatAsBoundary is for a caller that validates such a carrier's contents at the
    // point they are inserted, and therefore wants the walk to stop at the carrier rather than reject it.
    enum class ECk_UntracedStructSafety_GcTracedStructs : uint8
    {
        Reject,
        TreatAsBoundary,
    };

    // Every knob defaults to the fail-closed answer, so a caller that passes no policy gets the original contract.
    struct CKCORE_API FCk_UntracedStructSafety_Policy
    {
        ECk_UntracedStructSafety_ClassRefs ClassRefs = ECk_UntracedStructSafety_ClassRefs::Reject;
        ECk_UntracedStructSafety_GcTracedStructs GcTracedStructs = ECk_UntracedStructSafety_GcTracedStructs::Reject;
    };

    // Recursively classifies whether a reflected struct value may be retained in memory Unreal's GC does not visit.
    // Deliberately fail-closed: strong UObject/interface leaves require tracing, and native carriers whose reference
    // semantics reflection cannot prove are reported as UnprovenOpaque rather than accepted.
    CKCORE_API auto
    Analyze_UntracedStructSafety(
        const UScriptStruct* InStructType,
        const FCk_UntracedStructSafety_Policy& InPolicy = {}) -> FCk_UntracedStructSafetyResult;
}
