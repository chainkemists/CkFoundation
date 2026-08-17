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

    // Recursively classifies whether a reflected struct value may be retained in memory Unreal's GC does not visit.
    // Deliberately fail-closed: strong UObject/interface leaves require tracing, and native carriers whose reference
    // semantics reflection cannot prove are reported as UnprovenOpaque rather than accepted.
    CKCORE_API auto
    Analyze_UntracedStructSafety(
        const UScriptStruct* InStructType) -> FCk_UntracedStructSafetyResult;
}
