#pragma once

#include <CoreMinimal.h>
#include <Kismet/BlueprintFunctionLibrary.h>
#include <ProfilingDebugging/CpuProfilerTrace.h>
#include "CkCpuWork.generated.h"

// Fixed vocabulary: frame totals across all game-thread callers and worlds, not last-writer gauges.
UENUM(BlueprintType)
enum class ECk_CpuWorkCounter : uint8
{
    CrowdMembersVisited,
    CrowdFrozenSkips,
    CrowdIntervalSkips,
    CrowdZeroRateSkips,
    CrowdFrameChanges,
    CrowdVisibleFrameChanges,
    CrowdDirtyTilePushes,
    CrowdHighlightPushes,
    CrowdUploadCandidates,
    CrowdUploadedInstances,
    CosmeticBuckets,
    CosmeticsVisited,
    CosmeticsPruned,
    CosmeticSocketMisses,
    CosmeticTransformRequests,
    CosmeticTransformTargetUnchanged,
    CosmeticTransformTargetChanged,
    ComponentEntries,
    ComponentSetupRejected,
    ComponentPushDisabled,
    ComponentNonSceneRejected,
    ComponentMissingTransform,
    ComponentInvalid,
    ComponentUnchanged,
    ComponentChanged,
    ComponentStaticRebakes,
    SceneParentsVisited,
    SceneMissingRecord,
    SceneUnchangedParents,
    SceneChildrenVisited,
    LodEntities,
    LodHidden,
    LodPromoted,
    LodFading,
    LodSweepCalls,
    LodSweepSlots,
    LodSweepLiveOwners,
    LodSweepAlreadyFree,
    LodSweepReclaims,
    LodFarUpdates,
    LodPromotions,
    LodDemoteBegins,
    LodDemoteFinishes,
    LodFadeTicks,
    TransformRequestBatches,
    TransformRequestsDrained,
    TransformRequestsRejectedParentDriven,
    TransformRequestsCancelledUndrained,
    TransformRequestBatchesChanged,
    TransformRequestBatchesUnchanged,
    Count UMETA(Hidden)
};

UENUM(BlueprintType)
enum class ECk_CpuWorkPhase : uint8
{
    LodBatch,
    LodPoolSweep,
    LodFarUpdate,
    LodPromote,
    LodDemoteBegin,
    LodDemoteFinish,
    LodFade,
    Count UMETA(Hidden)
};

namespace ck::cpu_work
{
    CKPROFILE_API auto Get_Enabled() -> bool;
    CKPROFILE_API auto Add(ECk_CpuWorkCounter InCounter, int64 InValue) -> void;
    CKPROFILE_API auto Get_CurrentCount(ECk_CpuWorkCounter InCounter) -> int64;
    // Module-owned frame callbacks; no timers, worlds, entities or capture-controller ownership.
    CKPROFILE_API auto BeginFrame() -> void;
    CKPROFILE_API auto EndFrame() -> void;
}

// Fixed phase identity, balanced even when a script returns early. uint constructor avoids
// binding-order dependence on reflected enum registration; callers pass uint(ECk_CpuWorkPhase::...).
struct CKPROFILE_API FCk_CpuWorkScope
{
    explicit FCk_CpuWorkScope(uint32 InPhase);
    ~FCk_CpuWorkScope();
    FCk_CpuWorkScope(const FCk_CpuWorkScope&) = delete;
    auto operator=(const FCk_CpuWorkScope&) -> FCk_CpuWorkScope& = delete;
private:
#if CPUPROFILERTRACE_ENABLED
    TOptional<FCpuProfilerTrace::FEventScope> _Scope;
#endif
};

UCLASS()
class CKPROFILE_API UCk_Utils_CpuWork : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure, Category = "Ck|Profile")
    static bool Get_Enabled();
    UFUNCTION(BlueprintCallable, Category = "Ck|Profile")
    static void Add_Counter(ECk_CpuWorkCounter InCounter, int32 InValue);
};
