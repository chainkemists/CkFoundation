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

    // Recursively classifies whether a reflected struct value may be retained in memory Unreal's GC does not visit.
    // Deliberately fail-closed: strong UObject/interface leaves require tracing, and native carriers whose reference
    // semantics reflection cannot prove are reported as UnprovenOpaque rather than accepted.
    CKCORE_API auto
    Analyze_UntracedStructSafety(
        const UScriptStruct* InStructType) -> FCk_UntracedStructSafetyResult;
}
