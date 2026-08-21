#pragma once

#include "CkQueue/Queue/CkQueue_Fragment_Data.h"

#include <CoreMinimal.h>
#include <Templates/Function.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::queue::layout
{
    enum class EBuildOutcome : uint8
    {
        Success,
        NoViablePlacement,
        SearchBudgetExhausted
    };

    struct CKQUEUE_API FPlacement
    {
        int32 OriginIndex = INDEX_NONE;
        int32 OriginRank = INDEX_NONE;
        FTransform TargetWorldTransform = FTransform::Identity;
    };

    struct CKQUEUE_API FBuildResult
    {
        EBuildOutcome Outcome = EBuildOutcome::NoViablePlacement;
        TArray<FPlacement> Placements;
        int32 SearchNodesVisited = 0;

        auto IsSuccess() const -> bool
        { return Outcome == EBuildOutcome::Success; }
    };

    // The validator owns all environment knowledge. It receives the candidate and the preceding placement in the
    // same origin (none for the front), and must not retain either reference after returning.
    using FPlacementValidator = TFunctionRef<bool(FTransform&, const TOptional<FTransform>&)>;

    // Builds every requested placement or returns an empty, explicit failure result. No partially viable formation is
    // ever published. Origin assignment is fixed before layout search: least normalized load, then origin index.
    CKQUEUE_API auto
        Build(
            const FTransform& InOwnerWorldTransform,
            const TArray<FCk_Queue_Origin>& InOrigins,
            int32 InMemberCount,
            float InSpacingUu,
            int32 InMaxSearchNodes,
            ECk_Queue_LayoutAlgorithm InLayoutAlgorithm,
            FPlacementValidator InValidator)
        -> FBuildResult;
}
