#include "CkAggroTarget_Processor.h"

#include "CkAggro/CkAggro_Fragment.h"
#include "CkAggro/CkAggro_Scoring.h"

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
        auto       Owner = ck::StaticCast<FCk_Handle_Aggro>(InTargetInfo.Get_AggroOwner());
        const auto OwnerHasThreatParams = ck::IsValid(Owner) && Owner.Has<ck::FFragment_Aggro_ThreatParams>();

        // Resolve the seed threat: per-target override, else the owner default.
        auto InitialThreat = InThreatParams.Get_InitialThreat();
        if (InThreatParams.Get_InitialThreatMode() == ECk_Aggro_OverridePolicy::UseOwnerDefault && OwnerHasThreatParams)
        { InitialThreat = Owner.Get<ck::FFragment_Aggro_ThreatParams>().Get_InitialThreat(); }

        // Clamp into the owner's range, then honor an optional per-target maximum.
        if (OwnerHasThreatParams)
        { InitialThreat = Owner.Get<ck::FFragment_Aggro_ThreatParams>().Get_ThreatClampRange().Get_ClampedValue(InitialThreat); }
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
        const auto Now      = ck_aggro_target_processor::Get_Now(InTarget);
        auto       Owner    = ck::StaticCast<FCk_Handle_Aggro>(InTargetInfo.Get_AggroOwner());
        const auto OldThreat = InThreat._Threat;

        const auto OwnerReady = ck::IsValid(Owner)
            && Owner.Has<ck::FFragment_Aggro_ThreatParams>()
            && Owner.Has<ck::FFragment_Aggro_SpatialParams>()
            && Owner.Has<ck::FFragment_Aggro_ForgetParams>();

        if (OwnerReady)
        {
            const auto& OwnerThreat  = Owner.Get<ck::FFragment_Aggro_ThreatParams>();
            const auto& OwnerSpatial = Owner.Get<ck::FFragment_Aggro_SpatialParams>();
            const auto& OwnerForget  = Owner.Get<ck::FFragment_Aggro_ForgetParams>();

            const auto ClampMin     = OwnerThreat.Get_ThreatClampRange().Get_Min();
            const auto ClampMaxBase = OwnerThreat.Get_ThreatClampRange().Get_Max();
            const auto ClampMax     = InThreatParams.Get_MaximumThreatOverrideMode() == ECk_EnableDisable::Enable
                ? FMath::Min<double>(ClampMaxBase, InThreatParams.Get_MaximumThreatOverride())
                : ClampMaxBase;

            // Advance the analytic decay anchor up to Now before adding the delta.
            const auto Elapsed      = (Now - InThreat._LastDecayTime).Get_Seconds();
            const auto SecsSincePer = (Now - InPerception._LastPerceivedTime).Get_Seconds();
            const auto PerceptK     = ck_aggro::Compute_PerceptionDecayMultiplier(
                InTarget.Has<ck::FTag_AggroTarget_Perceived>(), SecsSincePer,
                OwnerForget.Get_LostSightGraceDuration().Get_Seconds(), OwnerThreat.Get_UnperceivedThreatDecayMultiplier());
            const auto RangeK       = ck_aggro::Compute_RangeDecayMultiplier(
                InTarget.Has<ck::FTag_AggroTarget_WithinRetention>(), OwnerSpatial.Get_OutOfRangeDecayMultiplier());

            InThreat._Threat = ck_aggro::Compute_DecayedThreat(
                InThreat._Threat, Elapsed, OwnerThreat.Get_ThreatDecayRate(), PerceptK, RangeK, ClampMin, ClampMax);

            const auto Delta = InRequest.Get_ThreatDelta() * InThreatParams.Get_ThreatMultiplier();
            InThreat._Threat = static_cast<float>(FMath::Clamp<double>(static_cast<double>(InThreat._Threat) + Delta, ClampMin, ClampMax));
        }
        else
        {
            InThreat._Threat = InThreat._Threat + InRequest.Get_ThreatDelta() * InThreatParams.Get_ThreatMultiplier();
        }

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
        auto       Owner     = ck::StaticCast<FCk_Handle_Aggro>(InTargetInfo.Get_AggroOwner());
        const auto OldThreat = InThreat._Threat;

        if (ck::IsValid(Owner) && Owner.Has<ck::FFragment_Aggro_ThreatParams>())
        {
            const auto& OwnerThreat  = Owner.Get<ck::FFragment_Aggro_ThreatParams>();
            const auto  ClampMin     = OwnerThreat.Get_ThreatClampRange().Get_Min();
            const auto  ClampMaxBase = OwnerThreat.Get_ThreatClampRange().Get_Max();
            const auto  ClampMax     = InThreatParams.Get_MaximumThreatOverrideMode() == ECk_EnableDisable::Enable
                ? FMath::Min<double>(ClampMaxBase, InThreatParams.Get_MaximumThreatOverride())
                : ClampMaxBase;

            InThreat._Threat = static_cast<float>(FMath::Clamp<double>(InRequest.Get_Threat(), ClampMin, ClampMax));
        }
        else
        {
            InThreat._Threat = InRequest.Get_Threat();
        }

        // Absolute set — reset the decay anchor to Now.
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
        // Counted increment — one sense reports this target perceived.
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
        // Counted decrement; a no-op at count 0. On the last release, stamp the grace-window anchor.
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
        // Clear the perception count outright regardless of how many senses had voted.
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
        // Explicit forget — always honored (bypasses CannotBeForgotten). The Forget processor completes it.
        InTarget.AddOrGet<ck::FTag_AggroTarget_PendingForget>();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_AggroTarget_Evaluate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InTarget,
            const FFragment_AggroTarget_ThreatParams& InThreatParams,
            const FFragment_AggroTarget_ScoreParams& InScoreParams,
            const FFragment_AggroTarget_LifetimeParams& InLifetimeParams,
            const FFragment_AggroTarget_TargetInfo& InTargetInfo,
            const FFragment_AggroTarget_Perception& InPerception,
            FFragment_AggroTarget_Threat& InThreat,
            FFragment_AggroTarget_Score& InScore)
        -> void
    {
        const auto Now     = ck_aggro_target_processor::Get_Now(InTarget);
        auto       Owner   = ck::StaticCast<FCk_Handle_Aggro>(InTargetInfo.Get_AggroOwner());
        const auto Tracked = ck::UAggroTarget_TrackedEntity_Utils::Get_StoredEntity(InTarget);

        const auto OwnerReady = ck::IsValid(Owner)
            && Owner.Has<ck::FFragment_Aggro_ThreatParams>()
            && Owner.Has<ck::FFragment_Aggro_SpatialParams>()
            && Owner.Has<ck::FFragment_Aggro_ForgetParams>();

        // Invalid tracked (or a gone owner) -> forget (bypasses CannotBeForgotten), stop.
        if (ck::Is_NOT_Valid(Tracked) || NOT OwnerReady)
        {
            InTarget.AddOrGet<ck::FTag_AggroTarget_PendingForget>();
            InTarget.Try_Remove<ck::FTag_AggroTarget_EvaluationPending>();
            return;
        }

        const auto& OwnerThreat  = Owner.Get<ck::FFragment_Aggro_ThreatParams>();
        const auto& OwnerSpatial = Owner.Get<ck::FFragment_Aggro_SpatialParams>();
        const auto& OwnerForget  = Owner.Get<ck::FFragment_Aggro_ForgetParams>();

        // Distance owner<->tracked. A transformless tracked entity is treated as co-located (always in range).
        const auto Distance = UCk_Utils_Transform_UE::Has(Tracked)
            ? FVector::Dist(UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentLocation(Owner),
                            UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentLocation(Tracked))
            : 0.0;

        // Retention-band tag (churn bounded to eval cadence at band crossings).
        const auto IsWithinRetention = Distance <= OwnerSpatial.Get_RetentionDistance();
        if (IsWithinRetention && NOT InTarget.Has<ck::FTag_AggroTarget_WithinRetention>())
        { InTarget.Add<ck::FTag_AggroTarget_WithinRetention>(); }
        else if (NOT IsWithinRetention && InTarget.Has<ck::FTag_AggroTarget_WithinRetention>())
        { InTarget.Try_Remove<ck::FTag_AggroTarget_WithinRetention>(); }

        // Analytic decay -> advance the anchor.
        const auto ClampMin     = OwnerThreat.Get_ThreatClampRange().Get_Min();
        const auto ClampMaxBase = OwnerThreat.Get_ThreatClampRange().Get_Max();
        const auto ClampMax     = InThreatParams.Get_MaximumThreatOverrideMode() == ECk_EnableDisable::Enable
            ? FMath::Min<double>(ClampMaxBase, InThreatParams.Get_MaximumThreatOverride())
            : ClampMaxBase;

        const auto Elapsed      = (Now - InThreat.Get_LastDecayTime()).Get_Seconds();
        const auto SecsSincePer = (Now - InPerception.Get_LastPerceivedTime()).Get_Seconds();
        const auto PerceptK     = ck_aggro::Compute_PerceptionDecayMultiplier(
            InTarget.Has<ck::FTag_AggroTarget_Perceived>(), SecsSincePer,
            OwnerForget.Get_LostSightGraceDuration().Get_Seconds(), OwnerThreat.Get_UnperceivedThreatDecayMultiplier());
        const auto RangeK       = ck_aggro::Compute_RangeDecayMultiplier(IsWithinRetention, OwnerSpatial.Get_OutOfRangeDecayMultiplier());

        InThreat._Threat = ck_aggro::Compute_DecayedThreat(
            InThreat._Threat, Elapsed, OwnerThreat.Get_ThreatDecayRate(), PerceptK, RangeK, ClampMin, ClampMax);
        InThreat._LastDecayTime = Now;

        // Score (raw; no incumbent bias baked in).
        const auto DistFactor   = ck_aggro::Compute_DistanceFactor(
            Distance, OwnerSpatial.Get_DistanceFalloffHalfDistance(), OwnerSpatial.Get_DistanceFalloffExponent());
        const auto NearbyFactor = ck_aggro::Compute_NearbyFactor(
            OwnerSpatial.Get_NearbyPreference() == ECk_EnableDisable::Enable, Distance,
            OwnerSpatial.Get_NearbyPreferenceDistance(), OwnerSpatial.Get_NearbyPreferenceMultiplier());

        InScore._Score    = static_cast<float>(ck_aggro::Compute_Score(
            InThreat._Threat, DistFactor, NearbyFactor, InScoreParams.Get_ScoreMultiplier(), InScoreParams.Get_ScoreBias()));
        InScore._Distance = static_cast<float>(Distance);

        // Forget checks (skip if CannotBeForgotten — invalid-tracked handled above).
        if (NOT InTarget.Has<ck::FTag_AggroTarget_CannotBeForgotten>())
        {
            const auto Age           = (Now - InTargetInfo.Get_CreationTime()).Get_Seconds();
            const auto LastActivity  = FMath::Max(InThreat.Get_LastThreatTime().Get_Seconds(), InPerception.Get_LastPerceivedTime().Get_Seconds());
            const auto SinceActivity = Now.Get_Seconds() - LastActivity;

            const auto ShouldForget =
                   InThreat._Threat < OwnerThreat.Get_MinimumTrackedThreat()
                || SinceActivity > OwnerForget.Get_ForgetDuration().Get_Seconds()
                || (OwnerForget.Get_MaximumTargetAgeMode() == ECk_EnableDisable::Enable && Age > OwnerForget.Get_MaximumTargetAge().Get_Seconds())
                || (InLifetimeParams.Get_MaximumLifetimeMode() == ECk_EnableDisable::Enable && Age > InLifetimeParams.Get_MaximumLifetime().Get_Seconds());

            if (ShouldForget)
            { InTarget.AddOrGet<ck::FTag_AggroTarget_PendingForget>(); }
        }

        // Score changed -> the owner should re-select.
        Owner.AddOrGet<ck::FTag_Aggro_SelectionPending>();

        InTarget.Try_Remove<ck::FTag_AggroTarget_EvaluationPending>();
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
        auto       Owner       = ck::StaticCast<FCk_Handle_Aggro>(InTargetInfo.Get_AggroOwner());
        const auto Tracked     = ck::UAggroTarget_TrackedEntity_Utils::Get_StoredEntity(InTarget);
        const auto Now         = ck_aggro_target_processor::Get_Now(InTarget);
        const auto FinalThreat = InThreat.Get_Threat();
        const auto OwnerValid  = ck::IsValid(Owner);

        // Derive the reason from state (+ the Evicted tag) — no stored reason field.
        auto Reason = ECk_Aggro_ForgetReason::Requested;
        if (InTarget.Has<ck::FTag_AggroTarget_Evicted>())
        { Reason = ECk_Aggro_ForgetReason::Evicted; }
        else if (ck::Is_NOT_Valid(Tracked))
        { Reason = ECk_Aggro_ForgetReason::TargetInvalid; }
        else if (InLifetimeParams.Get_MaximumLifetimeMode() == ECk_EnableDisable::Enable
                 && (Now - InTargetInfo.Get_CreationTime()).Get_Seconds() > InLifetimeParams.Get_MaximumLifetime().Get_Seconds())
        { Reason = ECk_Aggro_ForgetReason::MaxLifetimeReached; }
        else if (OwnerValid && Owner.Has<ck::FFragment_Aggro_ThreatParams>() && Owner.Has<ck::FFragment_Aggro_ForgetParams>())
        {
            const auto& OwnerThreat = Owner.Get<ck::FFragment_Aggro_ThreatParams>();
            const auto& OwnerForget = Owner.Get<ck::FFragment_Aggro_ForgetParams>();
            const auto  Age          = (Now - InTargetInfo.Get_CreationTime()).Get_Seconds();
            const auto  LastActivity = FMath::Max(InThreat.Get_LastThreatTime().Get_Seconds(), InPerception.Get_LastPerceivedTime().Get_Seconds());

            if (FinalThreat < OwnerThreat.Get_MinimumTrackedThreat())
            { Reason = ECk_Aggro_ForgetReason::ThreatDepleted; }
            else if (OwnerForget.Get_MaximumTargetAgeMode() == ECk_EnableDisable::Enable && Age > OwnerForget.Get_MaximumTargetAge().Get_Seconds())
            { Reason = ECk_Aggro_ForgetReason::MaxAgeReached; }
            else if ((Now.Get_Seconds() - LastActivity) > OwnerForget.Get_ForgetDuration().Get_Seconds())
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
                    Current._ActiveTarget = FCk_Handle_AggroTarget{};
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
        auto Owner = ck::StaticCast<FCk_Handle_Aggro>(InTargetInfo.Get_AggroOwner());

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
