#include "CkVisualLodArbiter_Utils.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkIskmRenderer/Renderer/CkIskm_BatchedCrowd_Actor.h"
#include "CkIskmRenderer/Renderer/CkIskm_RenderProfile_Utils.h"

#include "CkIskmRenderer/AnimCollection/CkIskmAnimCollection_Fragment_Data.h"

#include "CkVisualLod/CkVisualLod_Ranking.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_visual_lod_arbiter_utils
{
    auto
    Get_IsExhaustionPolicyValid(
        ECk_VisualLod_PoolExhaustionPolicy InPolicy) -> bool
    {
        switch (InPolicy)
        {
        case ECk_VisualLod_PoolExhaustionPolicy::PromoteInstead:
        case ECk_VisualLod_PoolExhaustionPolicy::Unrendered:
            return true;

        default:
            return false;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::visual_lod::
    Get_AreRuntimeTunersValid(
        const FCk_VisualLodArbiter_RuntimeTuners& InTuners)
    -> bool
{
    const auto PromoteDistance = InTuners.Get_PromoteDistance();
    const auto DemoteDistance = InTuners.Get_DemoteDistance();
    const auto FadeSeconds = InTuners.Get_FadeDuration().Get_Seconds();

    return FMath::IsFinite(PromoteDistance) && PromoteDistance >= 0.0f
        && FMath::IsFinite(DemoteDistance) && DemoteDistance >= 0.0f
        && PromoteDistance <= DemoteDistance
        && InTuners.Get_NearBudget() >= 0
        && InTuners.Get_LockBudget() >= 0
        && FMath::IsFinite(InTuners.Get_LockPromoteMaxDistance()) && InTuners.Get_LockPromoteMaxDistance() >= 0.0f
        && ck_visual_lod_arbiter_utils::Get_IsExhaustionPolicyValid(InTuners.Get_ExhaustionPolicy())
        && FMath::IsFinite(InTuners.Get_ViewConeMarginDeg()) && InTuners.Get_ViewConeMarginDeg() >= 0.0f
        && FMath::IsFinite(InTuners.Get_AlwaysInViewDistance()) && InTuners.Get_AlwaysInViewDistance() >= 0.0f
        && FMath::IsFinite(InTuners.Get_PreemptDistanceMargin()) && InTuners.Get_PreemptDistanceMargin() >= 0.0f
        && InTuners.Get_MaxPreemptsPerTick() >= 0
        && FMath::IsFinite(FadeSeconds) && FadeSeconds >= 0.0
        && FMath::IsFinite(InTuners.Get_FadeAnchorLeadFrames())
        && FMath::IsFinite(InTuners.Get_FadeAnchorBakeLagIntervals());
}

auto
    ck::visual_lod::
    Get_AreRuntimeTunersValid(
        const FCk_VisualLodArbiter_RuntimeTuners& InTuners,
        const UCk_VisualLodArbiter_Data& InConfig)
    -> bool
{
    if (NOT Get_AreRuntimeTunersValid(InTuners)
        || InTuners.Get_CrowdTuners().Num() != InConfig.Get_CrowdConfigs().Num())
    { return false; }

    for (auto CrowdIndex = 0; CrowdIndex < InTuners.Get_CrowdTuners().Num(); ++CrowdIndex)
    {
        const auto& CrowdTuners = InTuners.Get_CrowdTuners()[CrowdIndex];
        const auto& CrowdConfig = InConfig.Get_CrowdConfigs()[CrowdIndex];
        const auto Collection = CrowdConfig.Get_AnimCollection().Get();
        const auto CollectionIsValid = ck::IsValid(Collection);
        if (NOT CollectionIsValid)
        { return false; }

        // Runtime tuner validity is an authoring contract. A collection is intentionally unbaked until
        // its first crowd is created, so using the transient baked count rejects valid cold setup.
        const auto SequenceCount = Collection->Get_Sequences().Num();
        const auto RateMin = static_cast<float>(CrowdTuners.Get_MoveRateClamp().Get_Min());
        const auto RateMax = static_cast<float>(CrowdTuners.Get_MoveRateClamp().Get_Max());
        const auto CrowdIsValid = CrowdTuners.Get_IdleSequenceIndex() >= 0
            && CrowdTuners.Get_IdleSequenceIndex() < SequenceCount
            && CrowdTuners.Get_MoveSequenceIndex() >= 0
            && CrowdTuners.Get_MoveSequenceIndex() < SequenceCount
            && FMath::IsFinite(CrowdTuners.Get_MoveSpeedThreshold()) && CrowdTuners.Get_MoveSpeedThreshold() >= 0.0f
            && FMath::IsFinite(CrowdTuners.Get_MoveAuthoredSpeed()) && CrowdTuners.Get_MoveAuthoredSpeed() > 0.0f
            && FMath::IsFinite(RateMin) && FMath::IsFinite(RateMax) && RateMin >= 0.0f && RateMin <= RateMax
            && CrowdTuners.Get_RenderBands().Num() == CrowdConfig.Get_RenderBands().Num();
        if (NOT CrowdIsValid)
        { return false; }

        auto Ranges = TArray<FVisualLod_RenderBandRange>{};
        Ranges.Reserve(CrowdTuners.Get_RenderBands().Num());
        for (const auto& BandTuners : CrowdTuners.Get_RenderBands())
        {
            auto Range = FVisualLod_RenderBandRange{};
            Range._DistanceThreshold = BandTuners.Get_DistanceThreshold();
            Range._ReturnHysteresis = BandTuners.Get_ReturnHysteresis();
            Ranges.Add(Range);

            if (NOT iskm::Get_AreRuntimeProfileTunersValid(BandTuners.Get_ProfileTuners()))
            { return false; }
        }

        if (NOT Get_AreRenderBandsValid(Ranges))
        { return false; }
    }

    return true;
}

auto
    ck::visual_lod::
    MakeRuntimeTuners(
        const UCk_VisualLodArbiter_Data& InConfig)
    -> FCk_VisualLodArbiter_RuntimeTuners
{
    auto Tuners = FCk_VisualLodArbiter_RuntimeTuners{};
    Tuners.Set_PromoteDistance(InConfig.Get_PromoteDistance());
    Tuners.Set_DemoteDistance(InConfig.Get_DemoteDistance());
    Tuners.Set_NearBudget(InConfig.Get_NearBudget());
    Tuners.Set_LockBudget(InConfig.Get_LockBudget());
    Tuners.Set_LockPromoteMaxDistance(InConfig.Get_LockPromoteMaxDistance());
    Tuners.Set_ExhaustionPolicy(InConfig.Get_ExhaustionPolicy());
    Tuners.Set_ViewConeMarginDeg(InConfig.Get_ViewConeMarginDeg());
    Tuners.Set_AlwaysInViewDistance(InConfig.Get_AlwaysInViewDistance());
    Tuners.Set_PreemptDistanceMargin(InConfig.Get_PreemptDistanceMargin());
    Tuners.Set_MaxPreemptsPerTick(InConfig.Get_MaxPreemptsPerTick());
    Tuners.Set_FadeDuration(InConfig.Get_FadeDuration());
    Tuners.Set_FadeAnchorLeadFrames(InConfig.Get_FadeAnchorLeadFrames());
    Tuners.Set_FadeAnchorBakeLagIntervals(InConfig.Get_FadeAnchorBakeLagIntervals());

    auto CrowdTuners = TArray<FCk_VisualLod_RuntimeCrowdTuners>{};
    CrowdTuners.Reserve(InConfig.Get_CrowdConfigs().Num());
    for (const auto& CrowdConfig : InConfig.Get_CrowdConfigs())
    {
        auto CrowdTuner = FCk_VisualLod_RuntimeCrowdTuners{};
        CrowdTuner.Set_IdleSequenceIndex(CrowdConfig.Get_IdleSequenceIndex());
        CrowdTuner.Set_MoveSequenceIndex(CrowdConfig.Get_MoveSequenceIndex());
        CrowdTuner.Set_MoveSpeedThreshold(CrowdConfig.Get_MoveSpeedThreshold());
        CrowdTuner.Set_MoveAuthoredSpeed(CrowdConfig.Get_MoveAuthoredSpeed());
        CrowdTuner.Set_MoveRateClamp(CrowdConfig.Get_MoveRateClamp());

        auto BandTuners = TArray<FCk_VisualLod_RuntimeRenderBandTuners>{};
        BandTuners.Reserve(CrowdConfig.Get_RenderBands().Num());
        for (const auto& RenderBand : CrowdConfig.Get_RenderBands())
        {
            const auto Profile = RenderBand.Get_RendererProfile().Get();
            auto BandTuner = FCk_VisualLod_RuntimeRenderBandTuners{};
            BandTuner.Set_DistanceThreshold(RenderBand.Get_DistanceThreshold());
            BandTuner.Set_ReturnHysteresis(RenderBand.Get_ReturnHysteresis());
            if (ck::IsValid(Profile))
            { BandTuner.Set_ProfileTuners(iskm::MakeRuntimeProfileTuners(*Profile)); }
            BandTuners.Add(BandTuner);
        }

        CrowdTuner.Set_RenderBands(BandTuners);
        CrowdTuners.Add(CrowdTuner);
    }

    Tuners.Set_CrowdTuners(CrowdTuners);
    return Tuners;
}

auto
    ck::visual_lod::
    TrySetRuntimeTuners(
        FCk_VisualLodArbiter_RuntimeTuners& OutRuntimeTuners,
        const FCk_VisualLodArbiter_RuntimeTuners& InCandidate)
    -> bool
{
    if (NOT Get_AreRuntimeTunersValid(InCandidate))
    { return false; }

    OutRuntimeTuners = InCandidate;
    return true;
}

auto
    ck::visual_lod::
    TrySetRuntimeTuners(
        FCk_VisualLodArbiter_RuntimeTuners& OutRuntimeTuners,
        const FCk_VisualLodArbiter_RuntimeTuners& InCandidate,
        const UCk_VisualLodArbiter_Data& InConfig)
    -> bool
{
    if (NOT Get_AreRuntimeTunersValid(InCandidate, InConfig))
    { return false; }

    OutRuntimeTuners = InCandidate;
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VisualLodArbiter_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_VisualLodArbiter_ParamsData& InParams)
    -> FCk_Handle_VisualLodArbiter
{
    InHandle.Add<ck::FFragment_VisualLodArbiter_Params>(InParams);
    InHandle.Add<ck::FFragment_VisualLodArbiter_Current>();

    InHandle.Add<ck::FTag_VisualLodArbiter_NeedsSetup>();

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_VisualLodArbiter_UE, FCk_Handle_VisualLodArbiter, ck::FFragment_VisualLodArbiter_Current, ck::FFragment_VisualLodArbiter_Params);

auto
    UCk_Utils_VisualLodArbiter_UE::
    Has_Any(
        const FCk_Handle& InHandle)
    -> bool
{
    return Has(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VisualLodArbiter_UE::
    Get_Observer(
        const FCk_Handle_VisualLodArbiter& InHandle)
    -> FCk_Handle
{
    return InHandle.Get<ck::FFragment_VisualLodArbiter_Current>().Get_Observer();
}

auto
    UCk_Utils_VisualLodArbiter_UE::
    Get_PromotedCount(
        const FCk_Handle_VisualLodArbiter& InHandle)
    -> int32
{
    return InHandle.Get<ck::FFragment_VisualLodArbiter_Current>().Get_PromotedOwners().Num();
}

auto
    UCk_Utils_VisualLodArbiter_UE::
    Get_NearPromotedCount(
        const FCk_Handle_VisualLodArbiter& InHandle)
    -> int32
{
    return InHandle.Get<ck::FFragment_VisualLodArbiter_Current>().Get_NearPromotedCount();
}

auto
    UCk_Utils_VisualLodArbiter_UE::
    Get_LockedPromotedCount(
        const FCk_Handle_VisualLodArbiter& InHandle)
    -> int32
{
    return InHandle.Get<ck::FFragment_VisualLodArbiter_Current>().Get_LockedPromotedCount();
}

auto
    UCk_Utils_VisualLodArbiter_UE::
    Get_UnbudgetedPromotedCount(
        const FCk_Handle_VisualLodArbiter& InHandle)
    -> int32
{
    return InHandle.Get<ck::FFragment_VisualLodArbiter_Current>().Get_UnbudgetedPromotedCount();
}

auto
    UCk_Utils_VisualLodArbiter_UE::
    Get_Crowd(
        const FCk_Handle_VisualLodArbiter& InHandle,
        int32 InCrowdIndex)
    -> ACk_Iskm_BatchedCrowd_Actor*
{
    const auto& Current = InHandle.Get<ck::FFragment_VisualLodArbiter_Current>();

    if (NOT Current._Crowds.IsValidIndex(InCrowdIndex))
    { return nullptr; }

    return Current._Crowds[InCrowdIndex]._Crowd.Get();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VisualLodArbiter_UE::
    Get_LastView(
        const FCk_Handle_VisualLodArbiter& InHandle)
    -> ck::FVisualLod_LocalView
{
    return InHandle.Get<ck::FFragment_VisualLodArbiter_Current>().Get_LastView();
}

auto
    UCk_Utils_VisualLodArbiter_UE::
    Get_PromotesThisTick(
        const FCk_Handle_VisualLodArbiter& InHandle)
    -> int32
{
    return InHandle.Get<ck::FFragment_VisualLodArbiter_Current>().Get_PromotesThisTick();
}

auto
    UCk_Utils_VisualLodArbiter_UE::
    Get_DemotesThisTick(
        const FCk_Handle_VisualLodArbiter& InHandle)
    -> int32
{
    return InHandle.Get<ck::FFragment_VisualLodArbiter_Current>().Get_DemotesThisTick();
}

auto
    UCk_Utils_VisualLodArbiter_UE::
    Get_PreemptsThisTick(
        const FCk_Handle_VisualLodArbiter& InHandle)
    -> int32
{
    return InHandle.Get<ck::FFragment_VisualLodArbiter_Current>().Get_PreemptsThisTick();
}

auto
    UCk_Utils_VisualLodArbiter_UE::
    Get_IsFrozen(
        const FCk_Handle_VisualLodArbiter& InHandle)
    -> bool
{
    return InHandle.Has<ck::FTag_VisualLodArbiter_Frozen>();
}

auto
    UCk_Utils_VisualLodArbiter_UE::
    Get_RuntimeTuners(
        const FCk_Handle_VisualLodArbiter& InHandle)
    -> FCk_VisualLodArbiter_RuntimeTuners
{
    return InHandle.Get<ck::FFragment_VisualLodArbiter_Current>().Get_RuntimeTuners();
}

auto
    UCk_Utils_VisualLodArbiter_UE::
    Get_AreRuntimeTunersValid(
        const FCk_Handle_VisualLodArbiter& InHandle,
        const FCk_VisualLodArbiter_RuntimeTuners& InRuntimeTuners)
    -> bool
{
    if (ck::Is_NOT_Valid(InHandle))
    { return false; }

    const auto Config = InHandle.Get<ck::FFragment_VisualLodArbiter_Current>().Get_Config().Get();
    if (ck::Is_NOT_Valid(Config))
    { return false; }

    return ck::visual_lod::Get_AreRuntimeTunersValid(InRuntimeTuners, *Config);
}

auto
    UCk_Utils_VisualLodArbiter_UE::
    Request_SetFrozen(
        FCk_Handle_VisualLodArbiter& InHandle,
        bool InFrozen)
    -> void
{
    CK_CALLSTACK_RECORD(ck::FFragment_VisualLodArbiter_Requests, InHandle);

    const auto Request = FCk_Request_VisualLodArbiter_SetFrozen{
        InFrozen ? ECk_EnableDisable::Enable : ECk_EnableDisable::Disable};

    InHandle.AddOrGet<ck::FFragment_VisualLodArbiter_Requests>()._Requests.Emplace(Request);
}

auto
    UCk_Utils_VisualLodArbiter_UE::
    Get_NumCrowds(
        const FCk_Handle_VisualLodArbiter& InHandle)
    -> int32
{
    return InHandle.Get<ck::FFragment_VisualLodArbiter_Current>()._Crowds.Num();
}

auto
    UCk_Utils_VisualLodArbiter_UE::
    Get_CrowdPoolDebugInfo(
        const FCk_Handle_VisualLodArbiter& InHandle,
        int32 InCrowdIndex)
    -> FCk_VisualLodArbiter_CrowdPoolDebugInfo
{
    const auto& Current = InHandle.Get<ck::FFragment_VisualLodArbiter_Current>();

    if (NOT Current._Crowds.IsValidIndex(InCrowdIndex))
    { return {}; }

    const auto& Runtime = Current._Crowds[InCrowdIndex];

    auto Info = FCk_VisualLodArbiter_CrowdPoolDebugInfo{};
    Info.PoolSize  = Runtime._SlotOwners.Num();
    Info.FreeSlots = Runtime._FreeSlots.Num();
    Info.Crowd     = Runtime._Crowd;

    return Info;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VisualLodArbiter_UE::
    Request_SetObserver(
        FCk_Handle_VisualLodArbiter& InHandle,
        const FCk_Request_VisualLodArbiter_SetObserver& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_VisualLodArbiter
{
    CK_CALLSTACK_RECORD(ck::FFragment_VisualLodArbiter_Requests, InHandle);

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InHandle.AddOrGet<ck::FFragment_VisualLodArbiter_Requests>()._Requests.Emplace(InRequest);

    return InHandle;
}

auto
    UCk_Utils_VisualLodArbiter_UE::
    Request_ClearObserver(
        FCk_Handle_VisualLodArbiter& InHandle,
        const FCk_Request_VisualLodArbiter_ClearObserver& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_VisualLodArbiter
{
    CK_CALLSTACK_RECORD(ck::FFragment_VisualLodArbiter_Requests, InHandle);

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InHandle.AddOrGet<ck::FFragment_VisualLodArbiter_Requests>()._Requests.Emplace(InRequest);

    return InHandle;
}

auto
    UCk_Utils_VisualLodArbiter_UE::
    Request_SetRuntimeTuners(
        FCk_Handle_VisualLodArbiter& InHandle,
        const FCk_Request_VisualLodArbiter_SetRuntimeTuners& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_VisualLodArbiter
{
    CK_CALLSTACK_RECORD(ck::FFragment_VisualLodArbiter_Requests, InHandle);

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InHandle.AddOrGet<ck::FFragment_VisualLodArbiter_Requests>()._Requests.Emplace(InRequest);
    return InHandle;
}

auto
    UCk_Utils_VisualLodArbiter_UE::
    Request_ResetRuntimeTuners(
        FCk_Handle_VisualLodArbiter& InHandle,
        const FCk_Request_VisualLodArbiter_ResetRuntimeTuners& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_VisualLodArbiter
{
    CK_CALLSTACK_RECORD(ck::FFragment_VisualLodArbiter_Requests, InHandle);

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InHandle.AddOrGet<ck::FFragment_VisualLodArbiter_Requests>()._Requests.Emplace(InRequest);
    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VisualLodArbiter_UE::
    BindTo_OnCrowdCreated(
        FCk_Handle_VisualLodArbiter& InHandle,
        const FCk_Delegate_VisualLodArbiter_CrowdCreated& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_VisualLodArbiter
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnVisualLodArbiter_CrowdCreated, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InHandle;
}

auto
    UCk_Utils_VisualLodArbiter_UE::
    UnbindFrom_OnCrowdCreated(
        FCk_Handle_VisualLodArbiter& InHandle,
        const FCk_Delegate_VisualLodArbiter_CrowdCreated& InDelegate)
    -> FCk_Handle_VisualLodArbiter
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnVisualLodArbiter_CrowdCreated, InHandle, InDelegate);
    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------
