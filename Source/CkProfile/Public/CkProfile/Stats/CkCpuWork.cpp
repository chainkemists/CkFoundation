#include "CkCpuWork.h"

#include <HAL/IConsoleManager.h>
#include <ProfilingDebugging/CountersTrace.h>

#if WITH_ANGELSCRIPT_CK
#include "AngelscriptBinds.h"
#endif

namespace ck_cpu_work
{
    TAutoConsoleVariable<int32> CVarCpuWork(TEXT("ck.Perf.CpuWork"), 0,
        TEXT("Opt-in CPU work scopes and frame totals. Latched at frame start. 0=off, 1=on."));
    constexpr auto CounterCount = static_cast<uint32>(ECk_CpuWorkCounter::Count);
    constexpr auto PhaseCount = static_cast<uint32>(ECk_CpuWorkPhase::Count);
    int64 FrameCounts[CounterCount] = {};
    bool FrameEnabled = false;
    TRACE_DECLARE_INT_COUNTER(CpuWork_CrowdMembersVisited, TEXT("CkCpuWork_CrowdMembersVisited"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_CrowdFrozenSkips, TEXT("CkCpuWork_CrowdFrozenSkips"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_CrowdIntervalSkips, TEXT("CkCpuWork_CrowdIntervalSkips"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_CrowdZeroRateSkips, TEXT("CkCpuWork_CrowdZeroRateSkips"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_CrowdFrameChanges, TEXT("CkCpuWork_CrowdFrameChanges"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_CrowdVisibleFrameChanges, TEXT("CkCpuWork_CrowdVisibleFrameChanges"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_CrowdDirtyTilePushes, TEXT("CkCpuWork_CrowdDirtyTilePushes"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_CrowdHighlightPushes, TEXT("CkCpuWork_CrowdHighlightPushes"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_CrowdUploadCandidates, TEXT("CkCpuWork_CrowdUploadCandidates"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_CrowdUploadedInstances, TEXT("CkCpuWork_CrowdUploadedInstances"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_CosmeticBuckets, TEXT("CkCpuWork_CosmeticBuckets"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_CosmeticsVisited, TEXT("CkCpuWork_CosmeticsVisited"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_CosmeticsPruned, TEXT("CkCpuWork_CosmeticsPruned"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_CosmeticSocketMisses, TEXT("CkCpuWork_CosmeticSocketMisses"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_CosmeticTransformRequests, TEXT("CkCpuWork_CosmeticTransformRequests"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_ComponentEntries, TEXT("CkCpuWork_ComponentEntries"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_ComponentSetupRejected, TEXT("CkCpuWork_ComponentSetupRejected"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_ComponentPushDisabled, TEXT("CkCpuWork_ComponentPushDisabled"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_ComponentNonSceneRejected, TEXT("CkCpuWork_ComponentNonSceneRejected"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_ComponentMissingTransform, TEXT("CkCpuWork_ComponentMissingTransform"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_ComponentInvalid, TEXT("CkCpuWork_ComponentInvalid"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_ComponentUnchanged, TEXT("CkCpuWork_ComponentUnchanged"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_ComponentChanged, TEXT("CkCpuWork_ComponentChanged"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_ComponentStaticRebakes, TEXT("CkCpuWork_ComponentStaticRebakes"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_SceneParentsVisited, TEXT("CkCpuWork_SceneParentsVisited"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_SceneMissingRecord, TEXT("CkCpuWork_SceneMissingRecord"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_SceneUnchangedParents, TEXT("CkCpuWork_SceneUnchangedParents"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_SceneChildrenVisited, TEXT("CkCpuWork_SceneChildrenVisited"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_LodEntities, TEXT("CkCpuWork_LodEntities"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_LodHidden, TEXT("CkCpuWork_LodHidden"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_LodPromoted, TEXT("CkCpuWork_LodPromoted"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_LodFading, TEXT("CkCpuWork_LodFading"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_LodSweepCalls, TEXT("CkCpuWork_LodSweepCalls"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_LodSweepSlots, TEXT("CkCpuWork_LodSweepSlots"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_LodSweepLiveOwners, TEXT("CkCpuWork_LodSweepLiveOwners"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_LodSweepAlreadyFree, TEXT("CkCpuWork_LodSweepAlreadyFree"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_LodSweepReclaims, TEXT("CkCpuWork_LodSweepReclaims"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_LodFarUpdates, TEXT("CkCpuWork_LodFarUpdates"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_LodPromotions, TEXT("CkCpuWork_LodPromotions"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_LodDemoteBegins, TEXT("CkCpuWork_LodDemoteBegins"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_LodDemoteFinishes, TEXT("CkCpuWork_LodDemoteFinishes"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_LodFadeTicks, TEXT("CkCpuWork_LodFadeTicks"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_TransformRequestBatches, TEXT("CkCpuWork_TransformRequestBatches"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_TransformRequestsDrained, TEXT("CkCpuWork_TransformRequestsDrained"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_TransformRequestsRejectedParentDriven, TEXT("CkCpuWork_TransformRequestsRejectedParentDriven"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_TransformRequestsCancelledUndrained, TEXT("CkCpuWork_TransformRequestsCancelledUndrained"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_TransformRequestBatchesChanged, TEXT("CkCpuWork_TransformRequestBatchesChanged"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_TransformRequestBatchesUnchanged, TEXT("CkCpuWork_TransformRequestBatchesUnchanged"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_Enabled, TEXT("CkCpuWork_Enabled"));
    TRACE_DECLARE_INT_COUNTER(CpuWork_Frame, TEXT("CkCpuWork_Frame"));
#if CPUPROFILERTRACE_ENABLED
    uint32 PhaseIds[PhaseCount] = {};
    const ANSICHAR* const PhaseNames[] = {
        "CkCpuWork_LodBatch",
        "CkCpuWork_LodPoolSweep",
        "CkCpuWork_LodFarUpdate",
        "CkCpuWork_LodPromote",
        "CkCpuWork_LodDemoteBegin",
        "CkCpuWork_LodDemoteFinish",
        "CkCpuWork_LodFade",
    };
    static_assert(UE_ARRAY_COUNT(PhaseNames) == PhaseCount);
#endif
}

auto ck::cpu_work::Get_Enabled() -> bool
{
    using namespace ck_cpu_work;
    return IsInGameThread() && FrameEnabled;
}

auto ck::cpu_work::BeginFrame() -> void
{
    using namespace ck_cpu_work;
    if (!IsInGameThread()) { return; }
    FMemory::Memzero(FrameCounts);
#if CPUPROFILERTRACE_ENABLED && COUNTERSTRACE_ENABLED
    FrameEnabled = CVarCpuWork.GetValueOnGameThread() != 0;
#else
    FrameEnabled = false;
#endif
}

auto ck::cpu_work::Add(ECk_CpuWorkCounter InCounter, int64 InValue) -> void
{
    using namespace ck_cpu_work;
    const auto Index = static_cast<uint32>(InCounter);
    // Diagnostic input must not overflow or index rejected storage, even with ensures compiled out.
    if (!Get_Enabled() || Index >= CounterCount || InValue < 0) { return; }
    if (InValue > MAX_int64 - FrameCounts[Index]) { return; }
    FrameCounts[Index] += InValue;
}

auto ck::cpu_work::Get_CurrentCount(ECk_CpuWorkCounter InCounter) -> int64
{
    using namespace ck_cpu_work;
    const auto Index = static_cast<uint32>(InCounter);
    if (!Get_Enabled() || Index >= CounterCount) { return 0; }
    return FrameCounts[Index];
}

auto ck::cpu_work::EndFrame() -> void
{
    using namespace ck_cpu_work;
    if (!IsInGameThread()) { return; }
    // Emit the mode even when disabled, making off/on recordings distinguishable.
    TRACE_COUNTER_SET_ALWAYS(CpuWork_Enabled, FrameEnabled ? 1 : 0);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_Frame, static_cast<int64>(GFrameCounter));
    if (!FrameEnabled) { return; }
    TRACE_COUNTER_SET_ALWAYS(CpuWork_CrowdMembersVisited, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::CrowdMembersVisited)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_CrowdFrozenSkips, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::CrowdFrozenSkips)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_CrowdIntervalSkips, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::CrowdIntervalSkips)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_CrowdZeroRateSkips, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::CrowdZeroRateSkips)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_CrowdFrameChanges, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::CrowdFrameChanges)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_CrowdVisibleFrameChanges, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::CrowdVisibleFrameChanges)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_CrowdDirtyTilePushes, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::CrowdDirtyTilePushes)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_CrowdHighlightPushes, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::CrowdHighlightPushes)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_CrowdUploadCandidates, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::CrowdUploadCandidates)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_CrowdUploadedInstances, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::CrowdUploadedInstances)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_CosmeticBuckets, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::CosmeticBuckets)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_CosmeticsVisited, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::CosmeticsVisited)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_CosmeticsPruned, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::CosmeticsPruned)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_CosmeticSocketMisses, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::CosmeticSocketMisses)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_CosmeticTransformRequests, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::CosmeticTransformRequests)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_ComponentEntries, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::ComponentEntries)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_ComponentSetupRejected, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::ComponentSetupRejected)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_ComponentPushDisabled, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::ComponentPushDisabled)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_ComponentNonSceneRejected, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::ComponentNonSceneRejected)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_ComponentMissingTransform, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::ComponentMissingTransform)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_ComponentInvalid, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::ComponentInvalid)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_ComponentUnchanged, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::ComponentUnchanged)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_ComponentChanged, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::ComponentChanged)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_ComponentStaticRebakes, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::ComponentStaticRebakes)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_SceneParentsVisited, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::SceneParentsVisited)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_SceneMissingRecord, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::SceneMissingRecord)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_SceneUnchangedParents, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::SceneUnchangedParents)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_SceneChildrenVisited, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::SceneChildrenVisited)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_LodEntities, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::LodEntities)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_LodHidden, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::LodHidden)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_LodPromoted, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::LodPromoted)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_LodFading, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::LodFading)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_LodSweepCalls, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::LodSweepCalls)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_LodSweepSlots, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::LodSweepSlots)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_LodSweepLiveOwners, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::LodSweepLiveOwners)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_LodSweepAlreadyFree, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::LodSweepAlreadyFree)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_LodSweepReclaims, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::LodSweepReclaims)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_LodFarUpdates, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::LodFarUpdates)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_LodPromotions, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::LodPromotions)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_LodDemoteBegins, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::LodDemoteBegins)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_LodDemoteFinishes, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::LodDemoteFinishes)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_LodFadeTicks, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::LodFadeTicks)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_TransformRequestBatches, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::TransformRequestBatches)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_TransformRequestsDrained, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::TransformRequestsDrained)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_TransformRequestsRejectedParentDriven, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::TransformRequestsRejectedParentDriven)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_TransformRequestsCancelledUndrained, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::TransformRequestsCancelledUndrained)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_TransformRequestBatchesChanged, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::TransformRequestBatchesChanged)]);
    TRACE_COUNTER_SET_ALWAYS(CpuWork_TransformRequestBatchesUnchanged, FrameCounts[static_cast<uint32>(ECk_CpuWorkCounter::TransformRequestBatchesUnchanged)]);
    FrameEnabled = false;
}

FCk_CpuWorkScope::FCk_CpuWorkScope(uint32 InPhase)
{
    using namespace ck_cpu_work;
#if CPUPROFILERTRACE_ENABLED
    if (InPhase >= PhaseCount || !ck::cpu_work::Get_Enabled()) { return; }
    _Scope.Emplace(PhaseIds[InPhase], PhaseNames[InPhase], true, __FILE__, __LINE__);
#endif
}

FCk_CpuWorkScope::~FCk_CpuWorkScope() = default;

auto UCk_Utils_CpuWork::Get_Enabled() -> bool
{
    return ck::cpu_work::Get_Enabled();
}

auto UCk_Utils_CpuWork::Add_Counter(ECk_CpuWorkCounter InCounter, int32 InValue) -> void
{
    ck::cpu_work::Add(InCounter, InValue);
}

#if WITH_ANGELSCRIPT_CK
AS_FORCE_LINK const FAngelscriptBinds::FBind Bind_ck_CpuWorkScope(FAngelscriptBinds::EOrder::Late, []
{
    FAngelscriptBinds::FNamespace Ns(FString(TEXT("ck")));
    auto Bind = FAngelscriptBinds::ValueClass<FCk_CpuWorkScope>("CpuWorkScope", FBindFlags());
    Bind.Constructor("void f(uint InPhase)", [](FCk_CpuWorkScope* Address, uint32 InPhase)
    {
        new(Address) FCk_CpuWorkScope(InPhase);
    });
    Bind.Destructor("void f()", [](FCk_CpuWorkScope* Address)
    {
        Address->~FCk_CpuWorkScope();
    });
});
#endif
