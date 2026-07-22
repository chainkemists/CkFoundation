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

    // Recursively classifies whether a reflected struct value may be retained in memory that Unreal's GC does not
    // visit. The analysis is deliberately fail-closed: reflected strong UObject/interface leaves require tracing,
    // while native carriers whose reference semantics reflection cannot prove are reported as UnprovenOpaque.
    // Empty AngelScript-generated structs are GC-independent marker/tag values when both their reflected property
    // graph and complete AngelScript value-storage size are empty.
    CKCORE_API auto
    Analyze_UntracedStructSafety(
        const UScriptStruct* InStructType) -> FCk_UntracedStructSafetyResult;
}
