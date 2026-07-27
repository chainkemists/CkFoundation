#include "CkAggroTarget_Processor.h"

#include "CkAggro/CkAggro_Fragment.h"
#include "CkAggro/CkAggro_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Time/CkTime_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_AggroTarget_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_AggroTarget_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_AggroTarget_Evaluate);
CK_REGISTER_PROCESSOR(ck::FProcessor_AggroTarget_Forget);
CK_REGISTER_PROCESSOR(ck::FProcessor_AggroTarget_EndPlay);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_aggro_target_processor
{
    auto
    Get_Now(
        const FCk_Handle& InHandle)
        -> FCk_Time
    {
        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        const auto TimeParams = FCk_Utils_Time_GetWorldTime_Params{World};
        return UCk_Utils_Time_UE::Get_WorldTime(TimeParams).Get_WorldTime().Get_Time();
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_AggroTarget_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InTarget,
            const FFragment_AggroTarget_ThreatParams& InThreatParams,
            const FFragment_AggroTarget_LifetimeParams& InLifetimeParams,
            FFragment_AggroTarget_TargetInfo& InTargetInfo,
            FFragment_AggroTarget_Threat& InThreat,
            FFragment_AggroTarget_Perception& InPerception)
        -> void
    {
        InTarget.Remove<MarkedDirtyBy>();

        const auto Now   = ck_aggro_target_processor::Get_Now(InTarget);
        auto       Owner = UCk_Utils_Aggro_UE::Cast(InTargetInfo.Get_AggroOwner());

        auto InitialThreat = InThreatParams.Get_ThreatClampRange().Get_ClampedValue(InThreatParams.Get_InitialThreat());
        if (InThreatParams.Get_MaximumThreatOverrideMode() == ECk_EnableDisable::Enable)
        { InitialThreat = FMath::Min(InitialThreat, InThreatParams.Get_MaximumThreatOverride()); }

        InThreat._Threat         = InitialThreat;
        InThreat._LastThreatTime = Now;
        InThreat._LastDecayTime  = Now;

        InPerception._LastPerceivedTime = Now;

        InTargetInfo._CreationTime = Now;

        if (InLifetimeParams.Get_CanBecomeActiveTarget() == ECk_EnableDisable::Disable)
        { InTarget.AddOrGet<ck::FTag_AggroTarget_CannotBecomeActive>(); }
        if (InLifetimeParams.Get_CanBeForgotten() == ECk_EnableDisable::Disable)
        { InTarget.AddOrGet<ck::FTag_AggroTarget_CannotBeForgotten>(); }

        if (ck::IsValid(Owner))
        { UUtils_Signal_OnAggroTargetAcquired::Broadcast(Owner, MakePayload(Owner, InTarget)); }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_AggroTarget_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InTarget,
            const FFragment_AggroTarget_ThreatParams& InThreatParams,
            const FFragment_AggroTarget_TargetInfo& InTargetInfo,
            FFragment_AggroTarget_Threat& InThreat,
            FFragment_AggroTarget_Perception& InPerception,
            FFragment_AggroTarget_Requests& InRequests) const
        -> void
    {
        const auto RequestsCopy = InRequests._Requests;
        InRequests._Requests.Reset();

        algo::ForEachRequest(RequestsCopy, ck::Visitor(
        [&](const auto& InRequest) -> void
        {
            DoHandleRequest(InTarget, InThreatParams, InTargetInfo, InThreat, InPerception, InRequest);

            if (InRequest.Get_IsRequestHandleValid())
            { InRequest.GetAndDestroyRequestHandle(); }
        }), policy::DontResetContainer{});

        if (InRequests._Requests.IsEmpty())
        { InTarget.Remove<MarkedDirtyBy>(); }
    }

    auto
        FProcessor_AggroTarget_HandleRequests::
        DoHandleRequest(
            HandleType InTarget,
            const FFragment_AggroTarget_ThreatParams& InThreatParams,
            const FFragment_AggroTarget_TargetInfo& InTargetInfo,
            FFragment_AggroTarget_Threat& InThreat,
            FFragment_AggroTarget_Perception& InPerception,
            const FCk_Request_AggroTarget_AddThreat& InRequest)
        -> void
    {
        const auto Now       = ck_aggro_target_processor::Get_Now(InTarget);
        auto       Owner     = UCk_Utils_Aggro_UE::Cast(InTargetInfo.Get_AggroOwner());
        const auto OldThreat = InThreat._Threat;

        const auto& SpatialP = InTarget.Get<ck::FFragment_AggroTarget_SpatialParams>();
        const auto& ForgetP  = InTarget.Get<ck::FFragment_AggroTarget_ForgetParams>();

        const auto ClampMin     = InThreatParams.Get_ThreatClampRange().Get_Min();
        const auto ClampMaxBase = InThreatParams.Get_ThreatClampRange().Get_Max();
        const auto ClampMax     = InThreatParams.Get_MaximumThreatOverrideMode() == ECk_EnableDisable::Enable
            ? FMath::Min<double>(ClampMaxBase, InThreatParams.Get_MaximumThreatOverride())
            : ClampMaxBase;

        const auto Elapsed      = (Now - InThreat._LastDecayTime).Get_Seconds();
        const auto SecsSincePer = (Now - InPerception._LastPerceivedTime).Get_Seconds();
        const auto PerceptK     = UCk_Utils_Aggro_UE::Compute_PerceptionDecayMultiplier(
            InTarget.Has<ck::FTag_AggroTarget_Perceived>(), SecsSincePer,
            ForgetP.Get_LostSightGraceDuration().Get_Seconds(), InThreatParams.Get_UnperceivedThreatDecayMultiplier());
        const auto RangeK       = UCk_Utils_Aggro_UE::Compute_RangeDecayMultiplier(
            InTarget.Has<ck::FTag_AggroTarget_WithinRetention>(), SpatialP.Get_OutOfRangeDecayMultiplier());

        InThreat._Threat = UCk_Utils_Aggro_UE::Compute_DecayedThreat(
            InThreat._Threat, Elapsed, InThreatParams.Get_ThreatDecayRate(), PerceptK, RangeK, ClampMin, ClampMax);

        const auto Delta = InRequest.Get_ThreatDelta() * InThreatParams.Get_ThreatMultiplier();
        InThreat._Threat = static_cast<float>(FMath::Clamp<double>(static_cast<double>(InThreat._Threat) + Delta, ClampMin, ClampMax));

        InThreat._LastDecayTime = Now;
        InThreat._LastThreatTime = Now;

        if (InThreat._Threat != OldThreat)
        { UUtils_Signal_OnAggroThreatChanged::Broadcast(InTarget, MakePayload(InTarget, OldThreat, InThreat._Threat)); }

        InTarget.AddOrGet<ck::FTag_AggroTarget_EvaluationPending>();
        if (ck::IsValid(Owner))
        { Owner.AddOrGet<ck::FTag_Aggro_SelectionPending>(); }
    }

    auto
        FProcessor_AggroTarget_HandleRequests::
        DoHandleRequest(
            HandleType InTarget,
            const FFragment_AggroTarget_ThreatParams& InThreatParams,
            const FFragment_AggroTarget_TargetInfo& InTargetInfo,
            FFragment_AggroTarget_Threat& InThreat,
            FFragment_AggroTarget_Perception& InPerception,
            const FCk_Request_AggroTarget_SetThreat& InRequest)
        -> void
    {
        const auto Now       = ck_aggro_target_processor::Get_Now(InTarget);
        auto       Owner     = UCk_Utils_Aggro_UE::Cast(InTargetInfo.Get_AggroOwner());
        const auto OldThreat = InThreat._Threat;

        const auto ClampMin     = InThreatParams.Get_ThreatClampRange().Get_Min();
        const auto ClampMaxBase = InThreatParams.Get_ThreatClampRange().Get_Max();
        const auto ClampMax     = InThreatParams.Get_MaximumThreatOverrideMode() == ECk_EnableDisable::Enable
            ? FMath::Min<double>(ClampMaxBase, InThreatParams.Get_MaximumThreatOverride())
            : ClampMaxBase;

        InThreat._Threat = static_cast<float>(FMath::Clamp<double>(InRequest.Get_Threat(), ClampMin, ClampMax));

        InThreat._LastDecayTime  = Now;
        InThreat._LastThreatTime = Now;

        if (InThreat._Threat != OldThreat)
        { UUtils_Signal_OnAggroThreatChanged::Broadcast(InTarget, MakePayload(InTarget, OldThreat, InThreat._Threat)); }

        InTarget.AddOrGet<ck::FTag_AggroTarget_EvaluationPending>();
        if (ck::IsValid(Owner))
        { Owner.AddOrGet<ck::FTag_Aggro_SelectionPending>(); }
    }

    auto
        FProcessor_AggroTarget_HandleRequests::
        DoHandleRequest(
            HandleType InTarget,
            const FFragment_AggroTarget_ThreatParams& InThreatParams,
            const FFragment_AggroTarget_TargetInfo& InTargetInfo,
            FFragment_AggroTarget_Threat& InThreat,
            FFragment_AggroTarget_Perception& InPerception,
            const FCk_Request_AggroTarget_MarkPerceived& InRequest)
        -> void
    {
        InTarget.Add<ck::FTag_AggroTarget_Perceived>();
        InPerception._LastPerceivedTime = ck_aggro_target_processor::Get_Now(InTarget);

        if (InRequest.Get_KnownLocationMode() == ECk_EnableDisable::Enable)
        {
            InPerception._LastKnownLocation = InRequest.Get_KnownLocation();
        }
        else
        {
            const auto Tracked = ck::UAggroTarget_TrackedEntity_Utils::Get_StoredEntity(InTarget);
            if (UCk_Utils_Transform_UE::Has(Tracked))
            { InPerception._LastKnownLocation = UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentLocation(Tracked); }
        }
    }

    auto
        FProcessor_AggroTarget_HandleRequests::
        DoHandleRequest(
            HandleType InTarget,
            const FFragment_AggroTarget_ThreatParams& InThreatParams,
            const FFragment_AggroTarget_TargetInfo& InTargetInfo,
            FFragment_AggroTarget_Threat& InThreat,
            FFragment_AggroTarget_Perception& InPerception,
            const FCk_Request_AggroTarget_MarkUnperceived& InRequest)
        -> void
    {
        if (NOT InTarget.Has<ck::FTag_AggroTarget_Perceived>())
        { return; }

        InTarget.Remove<ck::FTag_AggroTarget_Perceived>();

        if (NOT InTarget.Has<ck::FTag_AggroTarget_Perceived>())
        { InPerception._LastPerceivedTime = ck_aggro_target_processor::Get_Now(InTarget); }
    }

    auto
        FProcessor_AggroTarget_HandleRequests::
        DoHandleRequest(
            HandleType InTarget,
            const FFragment_AggroTarget_ThreatParams& InThreatParams,
            const FFragment_AggroTarget_TargetInfo& InTargetInfo,
            FFragment_AggroTarget_Threat& InThreat,
            FFragment_AggroTarget_Perception& InPerception,
            const FCk_Request_AggroTarget_ResetPerception& InRequest)
        -> void
    {
        if (NOT InTarget.Has<ck::FTag_AggroTarget_Perceived>())
        { return; }

        InTarget.Try_Remove<ck::FTag_AggroTarget_Perceived>();
        InPerception._LastPerceivedTime = ck_aggro_target_processor::Get_Now(InTarget);
    }

    auto
        FProcessor_AggroTarget_HandleRequests::
        DoHandleRequest(
            HandleType InTarget,
            const FFragment_AggroTarget_ThreatParams& InThreatParams,
            const FFragment_AggroTarget_TargetInfo& InTargetInfo,
            FFragment_AggroTarget_Threat& InThreat,
            FFragment_AggroTarget_Perception& InPerception,
            const FCk_Request_AggroTarget_Forget& InRequest)
        -> void
    {
        InTarget.AddOrGet<ck::FTag_AggroTarget_PendingForget>();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_AggroTarget_Evaluate::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        // Game-thread hoist — a worker-thread UWorld time read is neither safe nor needed (one world per registry).
        _Now = ck_aggro_target_processor::Get_Now(this->_TransientEntity);
        TParallelProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_AggroTarget_Evaluate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InTarget,
            const FFragment_AggroTarget_ThreatParams& InThreatParams,
            const FFragment_AggroTarget_ScoreParams& InScoreParams,
            const FFragment_AggroTarget_LifetimeParams& InLifetimeParams,
            const FFragment_AggroTarget_SpatialParams& InSpatialParams,
            const FFragment_AggroTarget_ForgetParams& InForgetParams,
            const FFragment_AggroTarget_TargetInfo& InTargetInfo,
            const FFragment_AggroTarget_Perception& InPerception,
            FFragment_AggroTarget_Threat& InThreat,
            FFragment_AggroTarget_Score& InScore) const
        -> void
    {
        const auto Now = _Now;

        // Worker thread — registry reads only; every structural mutation below is DEFERRED through InTarget's per-task
        // command buffer. Rationale and full contract: CkAggro/CLAUDE.md.
        const auto SelfHandle = ck::MakeHandle(InTarget.Get_Entity(), _TransientEntity);
        auto       Owner      = UCk_Utils_Aggro_UE::Cast(InTargetInfo.Get_AggroOwner());
        const auto Tracked    = ck::UAggroTarget_TrackedEntity_Utils::Get_StoredEntity(SelfHandle);
        const auto OwnerValid = ck::IsValid(Owner);

        // Deliberately bypasses CannotBeForgotten. A missing owner is fine (standalone target); an invalid tracked is not.
        if (ck::Is_NOT_Valid(Tracked))
        {
            InTarget.DeferAddOrGet<ck::FTag_AggroTarget_PendingForget>();
            InTarget.DeferTry_Remove<ck::FTag_AggroTarget_EvaluationPending>();
            return;
        }

        // A standalone or transformless pair reads as co-located (0.0) — deliberately in range, not out of it.
        const auto Distance = (OwnerValid && UCk_Utils_Transform_UE::Has(Owner) && UCk_Utils_Transform_UE::Has(Tracked))
            ? FVector::Dist(UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentLocation(Owner),
                            UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentLocation(Tracked))
            : 0.0;

        // Has() reads pre-flush state and the flip is deferred — equivalent to the serial read-then-mutate only
        // because each target is a single task.
        const auto IsWithinRetention = Distance <= InSpatialParams.Get_RetentionDistance();
        if (IsWithinRetention && NOT InTarget.Has<ck::FTag_AggroTarget_WithinRetention>())
        { InTarget.DeferAdd<ck::FTag_AggroTarget_WithinRetention>(); }
        else if (NOT IsWithinRetention && InTarget.Has<ck::FTag_AggroTarget_WithinRetention>())
        { InTarget.DeferTry_Remove<ck::FTag_AggroTarget_WithinRetention>(); }

        const auto ClampMin     = InThreatParams.Get_ThreatClampRange().Get_Min();
        const auto ClampMaxBase = InThreatParams.Get_ThreatClampRange().Get_Max();
        const auto ClampMax     = InThreatParams.Get_MaximumThreatOverrideMode() == ECk_EnableDisable::Enable
            ? FMath::Min<double>(ClampMaxBase, InThreatParams.Get_MaximumThreatOverride())
            : ClampMaxBase;

        const auto Elapsed      = (Now - InThreat.Get_LastDecayTime()).Get_Seconds();
        const auto SecsSincePer = (Now - InPerception.Get_LastPerceivedTime()).Get_Seconds();
        const auto PerceptK     = UCk_Utils_Aggro_UE::Compute_PerceptionDecayMultiplier(
            InTarget.Has<ck::FTag_AggroTarget_Perceived>(), SecsSincePer,
            InForgetParams.Get_LostSightGraceDuration().Get_Seconds(), InThreatParams.Get_UnperceivedThreatDecayMultiplier());
        const auto RangeK       = UCk_Utils_Aggro_UE::Compute_RangeDecayMultiplier(IsWithinRetention, InSpatialParams.Get_OutOfRangeDecayMultiplier());

        InThreat._Threat = UCk_Utils_Aggro_UE::Compute_DecayedThreat(
            InThreat._Threat, Elapsed, InThreatParams.Get_ThreatDecayRate(), PerceptK, RangeK, ClampMin, ClampMax);
        InThreat._LastDecayTime = Now;

        const auto DistFactor   = UCk_Utils_Aggro_UE::Compute_DistanceFactor(
            Distance, InSpatialParams.Get_DistanceFalloffHalfDistance(), InSpatialParams.Get_DistanceFalloffExponent());
        const auto NearbyFactor = UCk_Utils_Aggro_UE::Compute_NearbyFactor(
            InSpatialParams.Get_NearbyPreference() == ECk_EnableDisable::Enable, Distance,
            InSpatialParams.Get_NearbyPreferenceDistance(), InSpatialParams.Get_NearbyPreferenceMultiplier());

        InScore._Score    = static_cast<float>(UCk_Utils_Aggro_UE::Compute_Score(
            InThreat._Threat, DistFactor, NearbyFactor, InScoreParams.Get_ScoreMultiplier(), InScoreParams.Get_ScoreBias()));
        InScore._Distance = static_cast<float>(Distance);

        if (NOT InTarget.Has<ck::FTag_AggroTarget_CannotBeForgotten>())
        {
            const auto Age           = (Now - InTargetInfo.Get_CreationTime()).Get_Seconds();
            const auto LastActivity  = FMath::Max(InThreat.Get_LastThreatTime().Get_Seconds(), InPerception.Get_LastPerceivedTime().Get_Seconds());
            const auto SinceActivity = Now.Get_Seconds() - LastActivity;

            const auto ShouldForget =
                   InThreat._Threat < InThreatParams.Get_MinimumTrackedThreat()
                || SinceActivity > InForgetParams.Get_ForgetDuration().Get_Seconds()
                || (InForgetParams.Get_MaximumTargetAgeMode() == ECk_EnableDisable::Enable && Age > InForgetParams.Get_MaximumTargetAge().Get_Seconds())
                || (InLifetimeParams.Get_MaximumLifetimeMode() == ECk_EnableDisable::Enable && Age > InLifetimeParams.Get_MaximumLifetime().Get_Seconds());

            if (ShouldForget)
            { InTarget.DeferAddOrGet<ck::FTag_AggroTarget_PendingForget>(); }
        }

        // Deferred, and safe to spam: N idempotent adds of a plain tag flush to one.
        if (OwnerValid)
        { InTarget.ReadEntity(Owner.Get_Entity()).DeferAddOrGet<ck::FTag_Aggro_SelectionPending>(); }

        InTarget.DeferTry_Remove<ck::FTag_AggroTarget_EvaluationPending>();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_AggroTarget_Forget::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InTarget,
            const FFragment_AggroTarget_ThreatParams& InThreatParams,
            const FFragment_AggroTarget_LifetimeParams& InLifetimeParams,
            const FFragment_AggroTarget_TargetInfo& InTargetInfo,
            const FFragment_AggroTarget_Perception& InPerception,
            const FFragment_AggroTarget_Threat& InThreat)
        -> void
    {
        auto       Owner       = UCk_Utils_Aggro_UE::Cast(InTargetInfo.Get_AggroOwner());
        const auto Tracked     = ck::UAggroTarget_TrackedEntity_Utils::Get_StoredEntity(InTarget);
        const auto Now         = ck_aggro_target_processor::Get_Now(InTarget);
        const auto FinalThreat = InThreat.Get_Threat();
        const auto OwnerValid  = ck::IsValid(Owner);

        auto Reason = ECk_Aggro_ForgetReason::Requested;
        if (InTarget.Has<ck::FTag_AggroTarget_Evicted>())
        { Reason = ECk_Aggro_ForgetReason::Evicted; }
        else if (ck::Is_NOT_Valid(Tracked))
        { Reason = ECk_Aggro_ForgetReason::TargetInvalid; }
        else if (InLifetimeParams.Get_MaximumLifetimeMode() == ECk_EnableDisable::Enable
                 && (Now - InTargetInfo.Get_CreationTime()).Get_Seconds() > InLifetimeParams.Get_MaximumLifetime().Get_Seconds())
        { Reason = ECk_Aggro_ForgetReason::MaxLifetimeReached; }
        else
        {
            const auto& ForgetP      = InTarget.Get<ck::FFragment_AggroTarget_ForgetParams>();
            const auto  Age          = (Now - InTargetInfo.Get_CreationTime()).Get_Seconds();
            const auto  LastActivity = FMath::Max(InThreat.Get_LastThreatTime().Get_Seconds(), InPerception.Get_LastPerceivedTime().Get_Seconds());

            if (FinalThreat < InThreatParams.Get_MinimumTrackedThreat())
            { Reason = ECk_Aggro_ForgetReason::ThreatDepleted; }
            else if (ForgetP.Get_MaximumTargetAgeMode() == ECk_EnableDisable::Enable && Age > ForgetP.Get_MaximumTargetAge().Get_Seconds())
            { Reason = ECk_Aggro_ForgetReason::MaxAgeReached; }
            else if ((Now.Get_Seconds() - LastActivity) > ForgetP.Get_ForgetDuration().Get_Seconds())
            { Reason = ECk_Aggro_ForgetReason::ThreatTimeout; }
        }

        if (OwnerValid)
        {
            UUtils_Signal_OnAggroTargetForgotten::Broadcast(Owner,
                MakePayload(Owner, FCk_Aggro_TargetForgottenInfo{Tracked, FinalThreat, Reason}));

            if (Owner.Has<ck::FFragment_Aggro_TargetMap>())
            { Owner.Get<ck::FFragment_Aggro_TargetMap>()._TargetsByTrackedEntity.Remove(Tracked); }

            if (Owner.Has<ck::FFragment_Aggro_Current>())
            {
                auto& Current = Owner.Get<ck::FFragment_Aggro_Current>();
                if (Current.Get_ActiveTarget() == InTarget)
                {
                    // Deliberately do NOT clear _ActiveTarget here — only stamp for re-selection.
                    //
                    // Clearing it silently swallowed the end-of-encounter edge: Selection reads
                    // Incumbent, finds it already empty, computes Changed = (Best != Incumbent),
                    // and when the forgotten target was the LAST one Best is also empty — so
                    // Changed is false and OnAggroActiveTargetChanged never fires. A consumer
                    // driving state off that signal could therefore learn about every switch but
                    // never learn the encounter ended, and would stay latched on a dead target.
                    //
                    // Leaving the (ineligible, about-to-die) handle in place lets Selection see a
                    // genuine transition and broadcast exactly ONCE with the correct old value:
                    // as a clean switch when another target remains, and as old -> invalid when
                    // the table empties. Broadcasting the clear here instead would double-fire on
                    // a switch (calm, then re-engage). Consumers are unaffected by the brief
                    // dangling handle: Get_ActiveTarget already returns something that fails
                    // ck::IsValid either way.
                    Owner.AddOrGet<ck::FTag_Aggro_SelectionPending>();
                }
            }
        }

        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InTarget);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_AggroTarget_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InTarget,
            const FFragment_AggroTarget_TargetInfo& InTargetInfo)
        -> void
    {
        auto Owner = UCk_Utils_Aggro_UE::Cast(InTargetInfo.Get_AggroOwner());

        if (ck::Is_NOT_Valid(Owner) || NOT Owner.Has<ck::FFragment_Aggro_TargetMap>())
        { return; }

        const auto TrackedEntity = ck::UAggroTarget_TrackedEntity_Utils::Get_StoredEntity(InTarget);
        Owner.Get<ck::FFragment_Aggro_TargetMap>()._TargetsByTrackedEntity.Remove(TrackedEntity);

        auto& Current = Owner.Get<ck::FFragment_Aggro_Current>();
        if (Current.Get_ActiveTarget() == InTarget)
        {
            Current._ActiveTarget = FCk_Handle_AggroTarget{};
            Owner.AddOrGet<ck::FTag_Aggro_SelectionPending>();
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
