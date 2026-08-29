#include "CkVisualLodArbiter_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkCamera/Camera/CkCamera_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/SceneNode/CkSceneNode_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkIskmRenderer/AnimCollection/CkIskmAnimCollection_Fragment_Data.h"
#include "CkIskmRenderer/Renderer/CkIskm_BatchedCrowd_Actor.h"
#include "CkIskmRenderer/Renderer/CkIskm_BatchedUtils.h"
#include "CkIskmRenderer/Renderer/CkIskmRenderer_Utils.h"
#include "CkIskmRenderer/Proxy/CkIskmProxy_Utils.h"

#include "CkPhysics/Velocity/CkVelocity_Utils.h"

#include "Animation/AnimSequenceBase.h"

#include "CkResourceLoader/CkResourceLoader_Utils.h"

#include "CkVisualLod/CkVisualLod_Log.h"
#include "CkVisualLod/CkVisualLod_Utils.h"
#include "CkVisualLod/CkVisualLodArbiter_Utils.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_VisualLodArbiter_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_VisualLodArbiter_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_VisualLodArbiter_Update);
CK_REGISTER_PROCESSOR(ck::FProcessor_VisualLodArbiter_CancelPendingRequests);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_VisualLodArbiter_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_VisualLodArbiter_Params& InParams,
            FFragment_VisualLodArbiter_Current& InCurrent)
        -> void
    {
        if (NOT InCurrent._LoadedAssets.Get_IsRequested())
        {
            InCurrent._LoadedAssets = UCk_Utils_ResourceLoader_UE::RequestLoad_RootedBatch(
                TEXT("VisualLodArbiter.Setup"), {InParams.Get_Config().ToSoftObjectPath()});
        }

        if (NOT InCurrent._LoadedAssets.Get_IsReady())
        {
            InHandle.AddOrGet<FTag_VisualLodArbiter_PendingAssetLoad>();
            return;
        }

        const auto ResolvedConfig = Cast<UCk_VisualLodArbiter_Data>(
            InCurrent._LoadedAssets.Get_ResolvedObject(InParams.Get_Config().ToSoftObjectPath()));
        const auto AssetsAreLoaded = NOT InCurrent._LoadedAssets.Get_HasFailed() && ck::IsValid(ResolvedConfig);

        CK_ENSURE_IF_NOT(AssetsAreLoaded,
            TEXT("Cannot setup VisualLodArbiter [{}] - loading its Config [{}] through CkResourceLoader failed"),
            InHandle, InParams.Get_Config().ToSoftObjectPath())
        {
            InCurrent._LoadedAssets = {};
            InHandle.Try_Remove<FTag_VisualLodArbiter_PendingAssetLoad>();
            InHandle.Remove<MarkedDirtyBy>();
            return;
        }

        InCurrent._Config = ResolvedConfig;
        InCurrent._Crowds.SetNum(ResolvedConfig->Get_CrowdConfigs().Num());

        // Two live arbiters with one domain tag would both claim the same members — catch the
        // misconfiguration at the second arbiter's setup, when both configs are resolvable
        InHandle.View<FFragment_VisualLodArbiter_Current, CK_IGNORE_PENDING_KILL>().ForEach(
        [&](FCk_Entity InOtherEntity, const FFragment_VisualLodArbiter_Current& InOtherCurrent)
        {
            if (InOtherEntity == InHandle.Get_Entity())
            { return; }

            const auto OtherConfig = InOtherCurrent.Get_Config().Get();
            if (ck::Is_NOT_Valid(OtherConfig))
            { return; }

            CK_ENSURE_IF_NOT(OtherConfig->Get_DomainTag() != ResolvedConfig->Get_DomainTag(),
                TEXT("Two VisualLodArbiters share the domain tag [{}] in one world — members will be claimed ambiguously"),
                ResolvedConfig->Get_DomainTag())
            {}
        });

        InHandle.Try_Remove<FTag_VisualLodArbiter_PendingAssetLoad>();
        InHandle.Remove<MarkedDirtyBy>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VisualLodArbiter_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_VisualLodArbiter_Current& InCurrent,
            FFragment_VisualLodArbiter_Requests& InRequests) const
        -> void
    {
        const auto RequestsCopy = InRequests._Requests;
        InRequests._Requests.Reset();

        algo::ForEachRequest(RequestsCopy, ck::Visitor(
        [&](const auto& InRequest) -> void
        {
            auto Result = ECk_Request_OperationResult::Failed;
            const auto Guard = MakeCompletionGuard(InRequest, InHandle, Result);

            DoHandleRequest(InHandle, InCurrent, InRequest);

            Result = ECk_Request_OperationResult::Succeeded;
        }), policy::DontResetContainer{});

        if (InRequests._Requests.IsEmpty())
        { InHandle.Remove<MarkedDirtyBy>(); }
    }

    auto
        FProcessor_VisualLodArbiter_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VisualLodArbiter_Current& InCurrent,
            const FCk_Request_VisualLodArbiter_SetObserver& InRequest)
        -> void
    {
        InCurrent._Observer = InRequest.Get_Observer();
    }

    auto
        FProcessor_VisualLodArbiter_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VisualLodArbiter_Current& InCurrent,
            const FCk_Request_VisualLodArbiter_ClearObserver& InRequest)
        -> void
    {
        InCurrent._Observer = FCk_Handle{};
    }

    auto
        FProcessor_VisualLodArbiter_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VisualLodArbiter_Current& InCurrent,
            const FCk_Request_VisualLodArbiter_SetFrozen& InRequest)
        -> void
    {
        if (InRequest.Get_Frozen() == ECk_EnableDisable::Enable)
        { InHandle.AddOrGet<FTag_VisualLodArbiter_Frozen>(); }
        else
        { InHandle.Try_Remove<FTag_VisualLodArbiter_Frozen>(); }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VisualLodArbiter_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_VisualLodArbiter_Params& InParams,
            FFragment_VisualLodArbiter_Current& InCurrent)
        -> void
    {
        CK_TRIGGER_ENSURE(TEXT("FProcessor_VisualLodArbiter_Update::ForEachEntity dispatched — the DoTick shadow was removed, "
            "and the promote path now creates entities inside a live view"));
    }

    auto
        FProcessor_VisualLodArbiter_Update::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        auto Visited = 0;

        auto ArbiterScratch = TArray<FCk_Handle_VisualLodArbiter>{};
        this->_TransientEntity.View<FFragment_VisualLodArbiter_Params, FFragment_VisualLodArbiter_Current, CK_IGNORE_PENDING_KILL>().ForEach(
        [&](FCk_Entity InEntity, const FFragment_VisualLodArbiter_Params&, FFragment_VisualLodArbiter_Current&)
        {
            auto Generic = ck::MakeHandle(InEntity, this->_TransientEntity);
            if (Generic.Has<FTag_VisualLodArbiter_NeedsSetup>())
            { return; }

            ArbiterScratch.Emplace(UCk_Utils_VisualLodArbiter_UE::CastChecked(Generic));
        });

        for (auto& Arbiter : ArbiterScratch)
        { Visited += DoUpdate_Arbiter(InDeltaT, Arbiter); }

        this->_LastVisitedCount = Visited;
    }

    auto
        FProcessor_VisualLodArbiter_Update::
        DoUpdate_Arbiter(
            FCk_Time InDeltaT,
            FCk_Handle_VisualLodArbiter& InArbiter)
        -> int32
    {
        auto& Current = InArbiter.Get<FFragment_VisualLodArbiter_Current>();

        const auto Config = Current._Config.Get();
        CK_ENSURE_IF_NOT(ck::IsValid(Config),
            TEXT("VisualLodArbiter [{}] lost its resolved config — the rooted batch should pin it"), InArbiter)
        { return 0; }

        auto Ctx = FUpdateContext{};
        Ctx._DeltaT  = InDeltaT;
        Ctx._Arbiter = InArbiter;
        Ctx._Current = &Current;
        Ctx._Config  = Config;
        Ctx._World   = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InArbiter);

        Ctx._View = DoResolve_View(InArbiter, Current, *Config);
        Current._LastView = Ctx._View;

        Current._PromotesThisTick = 0;
        Current._DemotesThisTick  = 0;
        Current._PreemptsThisTick = 0;

        // No view, no decisions: nothing promotes, demotes, or updates (dedicated server, or no
        // observer wired yet). Mirrors the BB drivers' whole-batch skip
        if (NOT Ctx._View._IsValid)
        { return 0; }

        if (InArbiter.Has<FTag_VisualLodArbiter_Frozen>())
        { return DoUpdate_ArbiterFrozen(Ctx); }

        DoGather_Members(Ctx);

        Current._Candidates.Reset();
        Current._Incumbents.Reset();

        for (auto Idx = 0; Idx < Ctx._Members.Num(); ++Idx)
        { DoProcess_Member(Ctx, Idx, Ctx._Members[Idx]); }

        DoApply_RankedFlips(Ctx);

        return Ctx._Members.Num();
    }

    auto
        FProcessor_VisualLodArbiter_Update::
        DoResolve_View(
            const FCk_Handle_VisualLodArbiter& InArbiter,
            const FFragment_VisualLodArbiter_Current& InCurrent,
            const UCk_VisualLodArbiter_Data& InConfig)
        -> FVisualLod_LocalView
    {
        auto View = FVisualLod_LocalView{};

        auto ViewInfo = FMinimalViewInfo{};
        auto Resolved = false;

        // An explicit observer wins (split-screen-proof, test-wireable); unset falls back to
        // local-view discovery. No view either way (dedicated server, editor world) ⇒ no-op
        if (const auto Observer = InCurrent.Get_Observer();
            ck::IsValid(Observer))
        {
            const auto Camera = UCk_Utils_Camera_UE::Cast(Observer);
            if (ck::IsValid(Camera))
            {
                ViewInfo = UCk_Utils_Camera_UE::Get_ViewInfo(Camera);
                Resolved = true;
            }
        }

        if (NOT Resolved)
        { Resolved = UCk_Utils_Camera_UE::TryGet_LocalViewInfo(InArbiter, ViewInfo); }

        if (NOT Resolved)
        { return View; }

        View._IsValid     = true;
        View._Location    = ViewInfo.Location;
        View._Forward     = ViewInfo.Rotation.Vector();
        View._CosHalfCone = FMath::Cos(FMath::DegreesToRadians(
            ViewInfo.FOV * 0.5f + InConfig.Get_ViewConeMarginDeg()));

        return View;
    }

    auto
        FProcessor_VisualLodArbiter_Update::
        DoGather_Members(
            FUpdateContext& InCtx)
        -> void
    {
        const auto MyTag = InCtx._Config->Get_DomainTag();

        InCtx._Arbiter.View<FFragment_VisualLod_Params, FFragment_VisualLod_Current, CK_IGNORE_PENDING_KILL>().ForEach(
        [&](FCk_Entity InEntity, const FFragment_VisualLod_Params& InParams, FFragment_VisualLod_Current& InMemberCurrent)
        {
            auto Generic = ck::MakeHandle(InEntity, InCtx._Arbiter);

            if (Generic.Has<FTag_VisualLod_NeedsSetup>() || Generic.Has<FTag_VisualLod_Suspended>())
            { return; }

            if (ck::IsValid(InMemberCurrent._Arbiter))
            {
                if (InMemberCurrent._Arbiter == InCtx._Arbiter)
                { InCtx._Members.Emplace(UCk_Utils_VisualLod_UE::CastChecked(Generic)); }
                return;
            }

            if (MyTag.IsValid() && InParams.Get_ArbiterTag() == MyTag)
            {
                InMemberCurrent._Arbiter = InCtx._Arbiter;
                InCtx._Members.Emplace(UCk_Utils_VisualLod_UE::CastChecked(Generic));
            }
        });
    }

    auto
        FProcessor_VisualLodArbiter_Update::
        DoUpdate_ArbiterFrozen(
            FUpdateContext& InCtx)
        -> int32
    {
        // Deliberately NOT DoGather_Members: that pass also claims unowned members by domain tag,
        // which is a resolution a frozen arbiter must not make. Only entities it already owns
        InCtx._Arbiter.View<FFragment_VisualLod_Params, FFragment_VisualLod_Current, CK_IGNORE_PENDING_KILL>().ForEach(
        [&](FCk_Entity InEntity, const FFragment_VisualLod_Params&, const FFragment_VisualLod_Current& InMemberCurrent)
        {
            auto Generic = ck::MakeHandle(InEntity, InCtx._Arbiter);

            if (Generic.Has<FTag_VisualLod_NeedsSetup>() || Generic.Has<FTag_VisualLod_Suspended>())
            { return; }

            if (InMemberCurrent.Get_Arbiter() == InCtx._Arbiter)
            { InCtx._Members.Emplace(UCk_Utils_VisualLod_UE::CastChecked(Generic)); }
        });

        // Outside the view: the recovery path destroys the proxy's scene node, and entity
        // destruction inside a live view is exactly what the DoTick shadow exists to avoid
        for (const auto& Member : InCtx._Members)
        { DoProcess_Member_Frozen(InCtx, Member); }

        return InCtx._Members.Num();
    }

    auto
        FProcessor_VisualLodArbiter_Update::
        DoProcess_Member(
            FUpdateContext& InCtx,
            int32 InScratchIdx,
            FCk_Handle_VisualLod InMember)
        -> void
    {
        auto& MemberCurrent = InMember.Get<FFragment_VisualLod_Current>();
        const auto& MemberParams = InMember.Get<FFragment_VisualLod_Params>();
        const auto& Config = *InCtx._Config;

        const auto MemberXfHandle = UCk_Utils_Transform_UE::Cast(InMember);
        if (ck::Is_NOT_Valid(MemberXfHandle))
        { return; }
        const auto MemberXf = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(MemberXfHandle);

        DoSweep_Step(InCtx);

        // Crowd actors are weakly held and may be reclaimed/recreated independently of their
        // member entities. A retained slot index belongs only to the exact crowd recorded at
        // acquisition; invalidate stale ownership so a far member can acquire from the
        // replacement pool instead of silently disappearing
        if (MemberCurrent._MemberIndex != INDEX_NONE)
        {
            const auto AssignedCrowd = MemberCurrent._Crowd.Get();
            const auto CrowdIndex    = MemberParams.Get_CrowdIndex();
            const auto CurrentCrowd  = InCtx._Current->_Crowds.IsValidIndex(CrowdIndex)
                ? InCtx._Current->_Crowds[CrowdIndex]._Crowd.Get()
                : nullptr;

            if (ck::Is_NOT_Valid(AssignedCrowd) || AssignedCrowd != CurrentCrowd)
            {
                MemberCurrent._MemberIndex   = INDEX_NONE;
                MemberCurrent._Crowd         = nullptr;
                MemberCurrent._FadePhase     = EVisualLod_FadePhase::None;
                MemberCurrent._FadeAlpha     = 1.0f;
                MemberCurrent._PreemptDemote = false;

                // The crossfade force-ends at the solid state, so a kept proxy must stop dithering —
                // otherwise it freezes at its last partway mask with no far member behind it
                DoWrite_ProxyFade(MemberCurrent, Config.Get_FadeNearCustomPrimitiveDataSlot(), 0.0f);

                // A promoted member keeps its proxy, but still needs a replacement slot parked
                // underneath so it can demote normally later
                if (MemberCurrent._Promoted && MemberParams.Get_PromotionMode() == ECk_VisualLod_PromotionMode::Managed)
                {
                    DoTryAcquire_Member(InCtx, InMember, MemberCurrent, MemberXf);
                    const auto ReplacementCrowd = MemberCurrent._Crowd.Get();
                    if (ck::IsValid(ReplacementCrowd) && MemberCurrent._MemberIndex != INDEX_NONE)
                    {
                        UCk_Utils_IskmBatched_UE::Set_CrowdMemberVisible(
                            ReplacementCrowd, MemberCurrent._MemberIndex, false);
                        UCk_Utils_IskmBatched_UE::Clear_CrowdMemberCosmetics(
                            ReplacementCrowd, MemberCurrent._MemberIndex);
                    }
                }
            }
        }

        // External systems can tear the proxy down (or take it and give it back) while the module
        // was suspended — fail closed to the far representation instead of ticking a corpse
        if (MemberCurrent._Promoted
            && (ck::Is_NOT_Valid(MemberCurrent._Proxy) || ck::Is_NOT_Valid(MemberCurrent._VisualNode)))
        {
            DoRecover_FailClosed(InCtx, InMember, MemberCurrent);
            return;
        }

        // A hidden member must hold NO render resource: a retained slot is invisible waste that
        // starves the fixed pool
        if (MemberCurrent._Hidden)
        {
            if (MemberCurrent._Promoted && MemberParams.Get_PromotionMode() == ECk_VisualLod_PromotionMode::Managed)
            { DoDemote_Begin(InCtx, InMember, MemberCurrent, MemberXf); }

            if (MemberCurrent._MemberIndex != INDEX_NONE && NOT MemberCurrent._Promoted)
            { DoRelease_Member(InCtx, InMember, MemberCurrent); }

            if (MemberCurrent._MemberIndex == INDEX_NONE && NOT MemberCurrent._Promoted)
            { return; }
        }

        // ---- Slot acquisition (lazy; also lazily stands the crowd up) ----
        if (MemberCurrent._MemberIndex == INDEX_NONE && NOT MemberCurrent._Promoted)
        {
            if (MemberParams.Get_PromotionMode() == ECk_VisualLod_PromotionMode::Managed)
            { DoTryAcquire_Member(InCtx, InMember, MemberCurrent, MemberXf); }

            if (MemberCurrent._MemberIndex == INDEX_NONE)
            {
                if (MemberParams.Get_PromotionMode() == ECk_VisualLod_PromotionMode::AlwaysPromoted
                    || InCtx._Config->Get_ExhaustionPolicy() == ECk_VisualLod_PoolExhaustionPolicy::PromoteInstead)
                {
                    DoPromote(InCtx, InMember, MemberCurrent, MemberXf, EChargeClass::Unbudgeted);
                    return;
                }

                CK_ENSURE_IF_NOT(false,
                    TEXT("VisualLod crowd pool exhausted for arbiter [{}] — member [{}] stays unrendered (policy: Unrendered)"),
                    InCtx._Arbiter, InMember)
                { return; }
            }
        }

        const auto Distance = static_cast<float>((MemberXf.GetLocation() - InCtx._View._Location).Size());
        const auto InView   = visual_lod::Get_IsInView(MemberXf.GetLocation(), InCtx._View,
            Config.Get_AlwaysInViewDistance(), Distance);

        // Retained for tooling: the rank inputs the two candidate/incumbent branches below read
        MemberCurrent._LastDistance = Distance;
        MemberCurrent._LastInView   = InView;

        // A held lock blocks demotion outright, with NO distance term: an SKMC committed to a
        // ragdoll keeps it for the whole downed window. The lock distance gates only STARTING
        // such a promote. Hidden members are excluded — there is nothing to show
        const auto LockHeld = MemberCurrent._PromoteLock > 0 && NOT MemberCurrent._Hidden;

        if (MemberCurrent._FadePhase != EVisualLod_FadePhase::None)
        {
            constexpr auto AllowReversal = true;
            DoTick_Fade(InCtx, InMember, MemberCurrent, MemberXf, Distance, LockHeld, AllowReversal);
            return;
        }

        if (MemberCurrent._Promoted)
        {
            // Keep the proxy's locomotion tracking the far-anim (a SpeedDriven member speeds up /
            // idles while promoted). No-op for Fixed, and for a game-overridden proxy the cache
            // stops us re-issuing over its choice unless the far resolution actually changes
            DoDrive_ProxyAnim(InCtx, InMember, MemberCurrent);

            // Refund on release rather than at demote: a recovered member standing next to the
            // camera is an ordinary near promote from here on, and must stop charging the lock budget
            if (MemberCurrent._PromotedViaLock && NOT LockHeld)
            {
                InCtx._Current->_LockedPromotedCount = FMath::Max(InCtx._Current->_LockedPromotedCount - 1, 0);
                InCtx._Current->_NearPromotedCount   = InCtx._Current->_NearPromotedCount + 1;
                MemberCurrent._PromotedViaLock = false;
            }

            if (MemberCurrent._MemberIndex != INDEX_NONE && NOT LockHeld
                && Distance > Config.Get_DemoteDistance())
            {
                DoDemote_Begin(InCtx, InMember, MemberCurrent, MemberXf);
                return;
            }

            // Distance-demote is the ONLY demote reason: out-of-view is not one, or spinning the
            // camera would mass-demote everything behind it. Preemption is the one way view enters
            // a demote, and it is rate-limited
            if (NOT MemberCurrent._PromotedViaLock && NOT MemberCurrent._PromotedUnbudgeted
                && MemberCurrent._MemberIndex != INDEX_NONE
                && NOT LockHeld && NOT MemberCurrent._Hidden)
            {
                auto Incumbent = FVisualLod_RankEntry{};
                Incumbent._Index    = InScratchIdx;
                Incumbent._Distance = Distance;
                Incumbent._InView   = InView;
                InCtx._Current->_Incumbents.Add(Incumbent);
            }
            return;
        }

        // Dormancy-hidden members don't spend a promote slot — there's nothing to show
        if (NOT MemberCurrent._Hidden)
        {
            const auto WantLocked = LockHeld
                && Distance < Config.Get_LockPromoteMaxDistance()
                && InCtx._Current->_LockedPromotedCount < Config.Get_LockBudget();

            // The lock promote stays inline: it spends its OWN reserved budget, so it never
            // competes with the near candidates the ranked pass selects
            if (WantLocked)
            {
                DoPromote(InCtx, InMember, MemberCurrent, MemberXf, EChargeClass::Locked);
                return;
            }

            if (MemberCurrent._MemberIndex != INDEX_NONE && Distance < Config.Get_PromoteDistance())
            {
                auto Candidate = FVisualLod_RankEntry{};
                Candidate._Index    = InScratchIdx;
                Candidate._Distance = Distance;
                Candidate._InView   = InView;
                InCtx._Current->_Candidates.Add(Candidate);
            }
        }

        DoUpdate_FarMember(InCtx, InMember, MemberCurrent, MemberXf);
    }

    auto
        FProcessor_VisualLodArbiter_Update::
        DoProcess_Member_Frozen(
            FUpdateContext& InCtx,
            FCk_Handle_VisualLod InMember)
        -> void
    {
        auto& MemberCurrent = InMember.Get<FFragment_VisualLod_Current>();

        // Freeze holds decisions, not correctness: an externally torn-down proxy still has to fail
        // closed to the far representation rather than be ticked as a corpse for the whole hold
        if (MemberCurrent._Promoted
            && (ck::Is_NOT_Valid(MemberCurrent._Proxy) || ck::Is_NOT_Valid(MemberCurrent._VisualNode)))
        {
            DoRecover_FailClosed(InCtx, InMember, MemberCurrent);
            return;
        }

        if (MemberCurrent._FadePhase == EVisualLod_FadePhase::None)
        { return; }

        const auto MemberXfHandle = UCk_Utils_Transform_UE::Cast(InMember);
        if (ck::Is_NOT_Valid(MemberXfHandle))
        { return; }
        const auto MemberXf = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(MemberXfHandle);

        const auto Distance = static_cast<float>((MemberXf.GetLocation() - InCtx._View._Location).Size());

        // The fade runs out on the course it started: both the lock re-aim and the band reversal
        // are promote/demote decisions, and a frozen arbiter makes none
        constexpr auto LockHeld      = false;
        constexpr auto AllowReversal = false;
        DoTick_Fade(InCtx, InMember, MemberCurrent, MemberXf, Distance, LockHeld, AllowReversal);
    }

    auto
        FProcessor_VisualLodArbiter_Update::
        DoApply_RankedFlips(
            FUpdateContext& InCtx)
        -> void
    {
        auto& Current = *InCtx._Current;

        if (Current._Candidates.IsEmpty())
        { return; }

        const auto FreeBudget = FMath::Max(InCtx._Config->Get_NearBudget() - Current._NearPromotedCount, 0);

        const auto Selection = visual_lod::Select_Flips(Current._Candidates, Current._Incumbents,
            FreeBudget, InCtx._Config->Get_MaxPreemptsPerTick(), InCtx._Config->Get_PreemptDistanceMargin());

        for (const auto ScratchIdx : Selection._PromoteIndices)
        {
            auto Member = InCtx._Members[ScratchIdx];
            auto& MemberCurrent = Member.Get<FFragment_VisualLod_Current>();

            const auto XfHandle = UCk_Utils_Transform_UE::Cast(Member);
            if (ck::Is_NOT_Valid(XfHandle))
            { continue; }

            DoPromote(InCtx, Member, MemberCurrent,
                UCk_Utils_Transform_UE::Get_EntityCurrentTransform(XfHandle), EChargeClass::Near);
        }

        for (const auto ScratchIdx : Selection._PreemptDemoteIndices)
        {
            auto Member = InCtx._Members[ScratchIdx];
            auto& MemberCurrent = Member.Get<FFragment_VisualLod_Current>();

            const auto XfHandle = UCk_Utils_Transform_UE::Cast(Member);
            if (ck::Is_NOT_Valid(XfHandle))
            { continue; }

            MemberCurrent._PreemptDemote = true;
            ++Current._PreemptsThisTick;

            DoDemote_Begin(InCtx, Member, MemberCurrent,
                UCk_Utils_Transform_UE::Get_EntityCurrentTransform(XfHandle));
        }
    }

    auto
        FProcessor_VisualLodArbiter_Update::
        DoEnsure_Crowd(
            FUpdateContext& InCtx,
            int32 InCrowdIndex)
        -> ACk_Iskm_BatchedCrowd_Actor*
    {
        auto& Current = *InCtx._Current;

        CK_ENSURE_IF_NOT(Current._Crowds.IsValidIndex(InCrowdIndex),
            TEXT("VisualLod member crowd index [{}] is outside arbiter [{}]'s [{}] configured crowds"),
            InCrowdIndex, InCtx._Arbiter, Current._Crowds.Num())
        { return nullptr; }

        auto& Runtime = Current._Crowds[InCrowdIndex];

        if (const auto Existing = Runtime._Crowd.Get();
            ck::IsValid(Existing))
        { return Existing; }

        const auto& CrowdConfig = InCtx._Config->Get_CrowdConfigs()[InCrowdIndex];

        if (NOT Runtime._LoadedAssets.Get_IsRequested())
        {
            Runtime._LoadedAssets = UCk_Utils_ResourceLoader_UE::RequestLoad_RootedBatch(
                TEXT("VisualLod.Crowd"), {CrowdConfig.Get_AnimCollection().ToSoftObjectPath()});
        }

        if (NOT Runtime._LoadedAssets.Get_IsReady())
        { return nullptr; }

        const auto Collection = Cast<UCk_IskmAnimCollection_Data>(
            Runtime._LoadedAssets.Get_ResolvedObject(CrowdConfig.Get_AnimCollection().ToSoftObjectPath()));
        const auto AssetsAreLoaded = NOT Runtime._LoadedAssets.Get_HasFailed() && ck::IsValid(Collection);

        CK_ENSURE_IF_NOT(AssetsAreLoaded,
            TEXT("Cannot stand up VisualLod crowd [{}] for arbiter [{}] - loading AnimCollection [{}] failed"),
            InCrowdIndex, InCtx._Arbiter, CrowdConfig.Get_AnimCollection().ToSoftObjectPath())
        {
            Runtime._LoadedAssets = {};
            return nullptr;
        }

        const auto Crowd = UCk_Utils_IskmBatched_UE::Create_Crowd(
            InCtx._World, Collection, CrowdConfig.Get_TileSize());
        if (ck::Is_NOT_Valid(Crowd))
        { return nullptr; }

        // Fixed pool, parked out of sight; slots are recycled as members come and go
        const auto ParkXf = FTransform{
            FRotator::ZeroRotator,
            FVector{0.0f, 0.0f, InCtx._Config->Get_ParkZ()},
            FVector::OneVector};

        for (auto Idx = 0; Idx < CrowdConfig.Get_PoolSize(); ++Idx)
        {
            UCk_Utils_IskmBatched_UE::Add_CrowdMember(
                Crowd, ParkXf, CrowdConfig.Get_IdleSequenceIndex(), 1.0f, static_cast<float>(Idx) * 0.137f);
        }
        UCk_Utils_IskmBatched_UE::Finalize_Crowd(Crowd);

        for (auto Idx = 0; Idx < CrowdConfig.Get_PoolSize(); ++Idx)
        { UCk_Utils_IskmBatched_UE::Set_CrowdMemberVisible(Crowd, Idx, false); }

        Runtime._Crowd = Crowd;
        Runtime._Collection = Collection;
        Runtime._FreeSlots.Reset();
        Runtime._SlotOwners.Reset();
        for (auto Idx = CrowdConfig.Get_PoolSize() - 1; Idx >= 0; --Idx)
        { Runtime._FreeSlots.Add(Idx); }
        Runtime._SlotOwners.SetNum(CrowdConfig.Get_PoolSize());

        // Synchronous: the game pushes slot override materials / default custom data NOW,
        // before any member can become visible in this crowd
        UUtils_Signal_OnVisualLodArbiter_CrowdCreated::Broadcast(InCtx._Arbiter,
            MakePayload(InCtx._Arbiter, InCrowdIndex));

        return Crowd;
    }

    auto
        FProcessor_VisualLodArbiter_Update::
        DoTryAcquire_Member(
            FUpdateContext& InCtx,
            FCk_Handle_VisualLod InMember,
            FFragment_VisualLod_Current& InMemberCurrent,
            const FTransform& InMemberXf)
        -> void
    {
        const auto CrowdIndex = InMember.Get<FFragment_VisualLod_Params>().Get_CrowdIndex();

        const auto Crowd = DoEnsure_Crowd(InCtx, CrowdIndex);
        if (ck::Is_NOT_Valid(Crowd))
        { return; }

        auto& Runtime = InCtx._Current->_Crowds[CrowdIndex];
        if (Runtime._FreeSlots.IsEmpty())
        { return; }

        const auto Index = Runtime._FreeSlots.Pop();
        Runtime._SlotOwners[Index] = InMember;

        const auto& CrowdConfig = InCtx._Config->Get_CrowdConfigs()[CrowdIndex];

        InMemberCurrent._MemberIndex          = Index;
        InMemberCurrent._Crowd                = Crowd;
        InMemberCurrent._CurrentSequenceIndex = CrowdConfig.Get_IdleSequenceIndex();
        InMemberCurrent._CurrentRate          = 1.0f;

        // Fade alpha MUST be written before the member is shown: with a masked crowd material an
        // unwritten fade slot reads 0 and clips the whole member invisible; slot recycling must
        // not leak a mid-fade alpha either
        DoWrite_MemberFade(Crowd, Index, InCtx._Config->Get_FadeCustomDataSlot(), 1.0f);

        // BEFORE the first visible frame — the game's window to write per-member cosmetics
        // (material slices, tints) and register far cosmetic followers
        UUtils_Signal_OnVisualLod_MemberAcquired::Broadcast(InMember, MakePayload(InMember, Index));

        UCk_Utils_IskmBatched_UE::Set_CrowdMemberTransform(Crowd, Index, InMemberXf);
        UCk_Utils_IskmBatched_UE::Set_CrowdMemberAnimation(Crowd, Index,
            CrowdConfig.Get_IdleSequenceIndex(), 1.0f, false);
        UCk_Utils_IskmBatched_UE::Set_CrowdMemberVisible(Crowd, Index, NOT InMemberCurrent._Hidden);
    }

    auto
        FProcessor_VisualLodArbiter_Update::
        DoRelease_Member(
            FUpdateContext& InCtx,
            FCk_Handle_VisualLod InMember,
            FFragment_VisualLod_Current& InMemberCurrent)
        -> void
    {
        const auto Index = InMemberCurrent._MemberIndex;
        const auto Crowd = InMemberCurrent._Crowd.Get();

        if (ck::IsValid(Crowd) && Index != INDEX_NONE)
        {
            UCk_Utils_IskmBatched_UE::Set_CrowdMemberVisible(Crowd, Index, false);
            UCk_Utils_IskmBatched_UE::Clear_CrowdMemberCosmetics(Crowd, Index);
        }

        DoRecycle_Slot(*InCtx._Current, InMember.Get<FFragment_VisualLod_Params>().Get_CrowdIndex(), InMember, Index);

        InMemberCurrent._MemberIndex = INDEX_NONE;
        InMemberCurrent._Crowd       = nullptr;
        // Seq/rate are per-slot state, not per-member: the next slot is written fresh at
        // acquisition, and a stale value here would suppress that write's change check
        InMemberCurrent._CurrentSequenceIndex = INDEX_NONE;
        InMemberCurrent._CurrentRate          = 1.0f;
        InMemberCurrent._FadePhase            = EVisualLod_FadePhase::None;
        InMemberCurrent._FadeAlpha            = 1.0f;
        InMemberCurrent._PreemptDemote        = false;

        UUtils_Signal_OnVisualLod_MemberReleased::Broadcast(InMember, MakePayload(InMember, Index));
    }

    auto
        FProcessor_VisualLodArbiter_Update::
        DoPromote(
            FUpdateContext& InCtx,
            FCk_Handle_VisualLod InMember,
            FFragment_VisualLod_Current& InMemberCurrent,
            const FTransform& InMemberXf,
            EChargeClass InChargeClass)
        -> void
    {
        const auto& MemberParams = InMember.Get<FFragment_VisualLod_Params>();

        const auto RendererSoft = InMemberCurrent._RendererOverride.IsNull()
            ? MemberParams.Get_Renderer()
            : InMemberCurrent._RendererOverride;

        CK_ENSURE_IF_NOT(NOT RendererSoft.IsNull(),
            TEXT("VisualLod member [{}] needs to promote but has no renderer configured"), InMember)
        { return; }

        // Async: a cold renderer defers the promote a few ticks (the member stays a candidate);
        // a resident asset resolves inline and promotes this tick
        if (NOT InMemberCurrent._LoadedAssets.Get_IsRequested())
        {
            InMemberCurrent._LoadedAssets = UCk_Utils_ResourceLoader_UE::RequestLoad_RootedBatch(
                TEXT("VisualLod.Promote"), {RendererSoft.ToSoftObjectPath()});
        }

        if (NOT InMemberCurrent._LoadedAssets.Get_IsReady())
        { return; }

        const auto RendererData = Cast<UCk_IskmRenderer_Data>(
            InMemberCurrent._LoadedAssets.Get_ResolvedObject(RendererSoft.ToSoftObjectPath()));
        const auto AssetsAreLoaded = NOT InMemberCurrent._LoadedAssets.Get_HasFailed() && ck::IsValid(RendererData);

        CK_ENSURE_IF_NOT(AssetsAreLoaded,
            TEXT("VisualLod member [{}] failed to load its renderer [{}] — promote abandoned"),
            InMember, RendererSoft.ToSoftObjectPath())
        {
            InMemberCurrent._LoadedAssets = {};
            return;
        }

        auto MemberXfHandle = UCk_Utils_Transform_UE::CastChecked(InMember);
        const auto Node = UCk_Utils_SceneNode_UE::Create(MemberXfHandle, FTransform::Identity);
        if (ck::Is_NOT_Valid(Node))
        { return; }

        auto NodeGeneric = Node.ConvertToHandle();

        const auto RendererHandle = UCk_Utils_IskmRenderer_UE::Add(NodeGeneric, RendererData);

        auto NodeXfHandle = UCk_Utils_Transform_UE::CastChecked(NodeGeneric);
        const auto ProxyParams = FCk_Fragment_IskmProxy_ParamsData{}
            .Set_Renderer(RendererHandle)
            .Set_SpawnTransform(InMemberXf);
        auto Proxy = UCk_Utils_IskmProxy_UE::Add(NodeXfHandle, ProxyParams);

        CK_ENSURE_IF_NOT(ck::IsValid(Proxy),
            TEXT("VisualLod member [{}] failed to create its promoted IskmProxy — failing closed to far"), InMember)
        {
            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(NodeGeneric);
            return;
        }

        InMemberCurrent._Proxy = Proxy;

        // Put the proxy on the far member's locomotion as a single looping sequence (no AnimBP)
        // BEFORE the game seam fires, so it walks exactly as it did at range. A game that wants a
        // richer look here (its own AnimBP, a montage) overrides it by enqueuing after us
        DoDrive_ProxyAnim(InCtx, InMember, InMemberCurrent);

        // The proxy is live: the game applies its look (wardrobe, material overrides, socket
        // followers, an animation override) synchronously here
        UUtils_Signal_OnVisualLod_Promoted::Broadcast(InMember, MakePayload(InMember, Proxy));

        if (InMemberCurrent._Hidden)
        { UCk_Utils_IskmProxy_UE::Request_SetVisibility(Proxy, false, FCk_Delegate_Request_OnCompleted{}); }

        const auto Crowd = InMemberCurrent._Crowd.Get();

        // The proxy owns cosmetics now — stop the crowd's advance from ALSO driving them, or the
        // two fight over the same entities each frame
        if (ck::IsValid(Crowd) && InMemberCurrent._MemberIndex != INDEX_NONE)
        { UCk_Utils_IskmBatched_UE::Clear_CrowdMemberCosmetics(Crowd, InMemberCurrent._MemberIndex); }

        // Don't hide the member — it dithers out under the SKMC; the fade owns member visibility
        // from here. No-slot promotes (exhaustion fallback) and hidden members keep instant behavior
        const auto Crossfading = ck::IsValid(Crowd)
            && InMemberCurrent._MemberIndex != INDEX_NONE
            && NOT InMemberCurrent._Hidden;

        if (Crossfading)
        {
            InMemberCurrent._FadePhase = EVisualLod_FadePhase::PromoteFade;
            InMemberCurrent._FadeAlpha = 1.0f;
        }

        // Declare the near mask before anything can render this proxy: fully dithered OUT over the
        // still-solid far member for a crossfade, solid for an unfaded promote that has no far
        // member to cross with. The request waits for IskmProxy Setup (which zero-fills the slot to
        // solid) and applies in the same drain, ahead of the proxy's first rendered frame — this
        // write is what keeps that first frame flash-free
        DoWrite_ProxyFade(InMemberCurrent, InCtx._Config->Get_FadeNearCustomPrimitiveDataSlot(),
            Crossfading ? InMemberCurrent._FadeAlpha : 0.0f);

        InMemberCurrent._Promoted           = true;
        InMemberCurrent._PromotedViaLock    = InChargeClass == EChargeClass::Locked;
        InMemberCurrent._PromotedUnbudgeted = InChargeClass == EChargeClass::Unbudgeted;
        InMemberCurrent._VisualNode         = NodeGeneric;

        InMember.AddOrGet<FTag_VisualLod_Promoted>();

        auto& ArbiterCurrent = *InCtx._Current;
        ArbiterCurrent._PromotedOwners.Add(InMember);
        ++ArbiterCurrent._PromotesThisTick;
        switch (InChargeClass)
        {
            case EChargeClass::Near:       ++ArbiterCurrent._NearPromotedCount; break;
            case EChargeClass::Locked:     ++ArbiterCurrent._LockedPromotedCount; break;
            case EChargeClass::Unbudgeted: ++ArbiterCurrent._UnbudgetedPromotedCount; break;
        }
    }

    auto
        FProcessor_VisualLodArbiter_Update::
        DoDemote_Begin(
            FUpdateContext& InCtx,
            FCk_Handle_VisualLod InMember,
            FFragment_VisualLod_Current& InMemberCurrent,
            const FTransform& InMemberXf)
        -> void
    {
        // A preempt-demote was already counted by the ranked pass that chose it — it is a ranking
        // outcome, not one of this tick's distance demotes
        if (NOT InMemberCurrent._PreemptDemote)
        { ++InCtx._Current->_DemotesThisTick; }

        // Hidden members don't animate a fade — tear the proxy down now; the member stays hidden
        if (InMemberCurrent._Hidden)
        {
            DoDemote_Finish(InCtx, InMember, InMemberCurrent);
            return;
        }

        const auto Crowd = InMemberCurrent._Crowd.Get();
        if (ck::Is_NOT_Valid(Crowd))
        {
            DoDemote_Finish(InCtx, InMember, InMemberCurrent);
            return;
        }

        const auto FadeSlot = InCtx._Config->Get_FadeCustomDataSlot();

        // Fade = 0 BEFORE Set_CrowdMemberVisible(true), so the member's first visible frame is
        // fully dissolved — no 1-frame solid pop beside the still-present SKMC
        DoWrite_MemberFade(Crowd, InMemberCurrent._MemberIndex, FadeSlot, 0.0f);

        UCk_Utils_IskmBatched_UE::Set_CrowdMemberTransform(Crowd, InMemberCurrent._MemberIndex, InMemberXf);

        // Seq/rate from CURRENT velocity — never a hard idle reset (the idle-snap fix)
        const auto& CrowdConfig = InCtx._Config->Get_CrowdConfigs()[InMember.Get<FFragment_VisualLod_Params>().Get_CrowdIndex()];
        const auto [Seq, Rate] = DoCompute_FarAnim(InMemberCurrent._FarAnim, CrowdConfig, DoGet_PlanarSpeed(InMember));
        UCk_Utils_IskmBatched_UE::Set_CrowdMemberAnimation(Crowd, InMemberCurrent._MemberIndex, Seq, Rate, false);
        InMemberCurrent._CurrentSequenceIndex = Seq;
        InMemberCurrent._CurrentRate          = Rate;

        UCk_Utils_IskmBatched_UE::Set_CrowdMemberVisible(Crowd, InMemberCurrent._MemberIndex, true);

        InMemberCurrent._FadePhase = EVisualLod_FadePhase::DemoteFade;
        InMemberCurrent._FadeAlpha = 0.0f;

        // The proxy starts this fade solid and dithers OUT as the member comes back
        DoWrite_ProxyFade(InMemberCurrent, InCtx._Config->Get_FadeNearCustomPrimitiveDataSlot(),
            InMemberCurrent._FadeAlpha);
    }

    auto
        FProcessor_VisualLodArbiter_Update::
        DoDemote_Finish(
            FUpdateContext& InCtx,
            FCk_Handle_VisualLod InMember,
            FFragment_VisualLod_Current& InMemberCurrent)
        -> void
    {
        // The proxy is still live: the game detaches its socket followers, parks attach points,
        // and re-registers far cosmetics against the member slot — synchronously, before teardown
        UUtils_Signal_OnVisualLod_DemoteFinishing::Broadcast(InMember,
            MakePayload(InMember, InMemberCurrent._MemberIndex));

        if (ck::IsValid(InMemberCurrent._VisualNode))
        {
            auto Node = InMemberCurrent._VisualNode;
            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(Node);
        }

        DoRefund_Charge(*InCtx._Current, InMember, InMemberCurrent);

        InMemberCurrent._Promoted           = false;
        InMemberCurrent._PromotedViaLock    = false;
        InMemberCurrent._PromotedUnbudgeted = false;
        InMemberCurrent._VisualNode         = FCk_Handle{};
        InMemberCurrent._Proxy              = FCk_Handle_IskmProxy{};
        InMemberCurrent._ProxySequenceIndex = INDEX_NONE;
        InMemberCurrent._ProxyRate          = 1.0f;
        InMemberCurrent._FadePhase          = EVisualLod_FadePhase::None;
        InMemberCurrent._PreemptDemote      = false;

        InMember.Try_Remove<FTag_VisualLod_Promoted>();
    }

    auto
        FProcessor_VisualLodArbiter_Update::
        DoRecover_FailClosed(
            FUpdateContext& InCtx,
            FCk_Handle_VisualLod InMember,
            FFragment_VisualLod_Current& InMemberCurrent)
        -> void
    {
        // Fail closed to the retained far representation. A partial near path never keeps an
        // orphaned node, a hidden member, or a consumed budget slot
        if (ck::IsValid(InMemberCurrent._VisualNode))
        {
            auto Node = InMemberCurrent._VisualNode;
            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(Node);
        }

        DoRefund_Charge(*InCtx._Current, InMember, InMemberCurrent);

        InMemberCurrent._Promoted           = false;
        InMemberCurrent._PromotedViaLock    = false;
        InMemberCurrent._PromotedUnbudgeted = false;
        InMemberCurrent._VisualNode         = FCk_Handle{};
        InMemberCurrent._Proxy              = FCk_Handle_IskmProxy{};
        InMemberCurrent._ProxySequenceIndex = INDEX_NONE;
        InMemberCurrent._ProxyRate          = 1.0f;
        InMemberCurrent._FadePhase          = EVisualLod_FadePhase::None;
        InMemberCurrent._FadeAlpha          = 1.0f;
        InMemberCurrent._PreemptDemote      = false;

        InMember.Try_Remove<FTag_VisualLod_Promoted>();

        const auto Crowd = InMemberCurrent._Crowd.Get();
        if (ck::IsValid(Crowd) && InMemberCurrent._MemberIndex != INDEX_NONE)
        {
            DoWrite_MemberFade(Crowd, InMemberCurrent._MemberIndex, InCtx._Config->Get_FadeCustomDataSlot(), 1.0f);
            UCk_Utils_IskmBatched_UE::Set_CrowdMemberVisible(Crowd, InMemberCurrent._MemberIndex,
                NOT InMemberCurrent._Hidden);

            // The member is the sole representation again — the game re-registers far cosmetics
            UUtils_Signal_OnVisualLod_DemoteFinishing::Broadcast(InMember,
                MakePayload(InMember, InMemberCurrent._MemberIndex));
        }
    }

    auto
        FProcessor_VisualLodArbiter_Update::
        DoTick_Fade(
            FUpdateContext& InCtx,
            FCk_Handle_VisualLod InMember,
            FFragment_VisualLod_Current& InMemberCurrent,
            const FTransform& InMemberXf,
            float InDistance,
            bool InLockHeld,
            bool InAllowReversal)
        -> void
    {
        const auto Crowd    = InMemberCurrent._Crowd.Get();
        const auto FadeSlot = InCtx._Config->Get_FadeCustomDataSlot();
        const auto NearSlot = InCtx._Config->Get_FadeNearCustomPrimitiveDataSlot();

        // 1. Member vanished mid-fade (slot reclaimed / crowd gone): resolve the promote state
        if (ck::Is_NOT_Valid(Crowd) || InMemberCurrent._MemberIndex == INDEX_NONE)
        {
            if (InMemberCurrent._FadePhase == EVisualLod_FadePhase::DemoteFade)
            { DoDemote_Finish(InCtx, InMember, InMemberCurrent); }
            else
            {
                // Proxy is now the sole representation, steady and solid — stop dithering it
                DoWrite_ProxyFade(InMemberCurrent, NearSlot, 0.0f);
                InMemberCurrent._FadePhase = EVisualLod_FadePhase::None;
            }
            return;
        }

        // 2. Dormancy hid the member mid-fade: snap it out, leave the fade slot clean at 1.0
        if (InMemberCurrent._Hidden)
        {
            DoWrite_ProxyFade(InMemberCurrent, NearSlot, 0.0f);
            UCk_Utils_IskmBatched_UE::Set_CrowdMemberVisible(Crowd, InMemberCurrent._MemberIndex, false);
            DoWrite_MemberFade(Crowd, InMemberCurrent._MemberIndex, FadeSlot, 1.0f);
            InMemberCurrent._FadeAlpha = 1.0f;
            if (InMemberCurrent._FadePhase == EVisualLod_FadePhase::DemoteFade)
            { DoDemote_Finish(InCtx, InMember, InMemberCurrent); }
            else
            { InMemberCurrent._FadePhase = EVisualLod_FadePhase::None; }
            return;
        }

        // 3. Reversal: crossing back over the band flips the fade from its current alpha — no
        //    proxy/slot churn, so band oscillation is nearly free. A lock taken mid-demote
        //    outranks the distance test outright (a downed member has to come BACK to the proxy);
        //    it also clears a preempt demote. A preempted member is inside the promote band by
        //    construction, so the near-side reversal must not see it. A frozen arbiter passes
        //    InAllowReversal false — re-aiming a fade is a decision, and it makes none
        if (InAllowReversal)
        {
            if (InLockHeld)
            {
                InMemberCurrent._FadePhase     = EVisualLod_FadePhase::PromoteFade;
                InMemberCurrent._PreemptDemote = false;
            }
            else if (InMemberCurrent._FadePhase == EVisualLod_FadePhase::PromoteFade
                && InDistance > InCtx._Config->Get_DemoteDistance())
            { InMemberCurrent._FadePhase = EVisualLod_FadePhase::DemoteFade; }
            else if (InMemberCurrent._FadePhase == EVisualLod_FadePhase::DemoteFade
                && NOT InMemberCurrent._PreemptDemote
                && InDistance < InCtx._Config->Get_PromoteDistance())
            { InMemberCurrent._FadePhase = EVisualLod_FadePhase::PromoteFade; }
        }

        // 4. Step alpha toward the phase target
        const auto FadeSeconds = static_cast<float>(InCtx._Config->Get_FadeDuration().Get_Seconds());
        const auto Step = FadeSeconds > 0.0f
            ? static_cast<float>(InCtx._DeltaT.Get_Seconds()) / FadeSeconds
            : 1.0f;
        if (InMemberCurrent._FadePhase == EVisualLod_FadePhase::PromoteFade)
        { InMemberCurrent._FadeAlpha = FMath::Max(InMemberCurrent._FadeAlpha - Step, 0.0f); }
        else
        { InMemberCurrent._FadeAlpha = FMath::Min(InMemberCurrent._FadeAlpha + Step, 1.0f); }

        // 5. Push the current alpha to BOTH sides of the crossfade — one value, two complementary
        //    dither masks: the far member solidifies as it rises, the near proxy dithers out
        DoWrite_MemberFade(Crowd, InMemberCurrent._MemberIndex, FadeSlot, InMemberCurrent._FadeAlpha);
        DoWrite_ProxyFade(InMemberCurrent, NearSlot, InMemberCurrent._FadeAlpha);

        // 6. Keep the member walking with the entity while it dissolves
        DoUpdate_FarMember(InCtx, InMember, InMemberCurrent, InMemberXf);

        // 7. Completion
        if (InMemberCurrent._FadePhase == EVisualLod_FadePhase::PromoteFade && InMemberCurrent._FadeAlpha <= 0.0f)
        {
            // Steady near state: member hidden under the SKMC, slot left clean at fade = 1.0
            UCk_Utils_IskmBatched_UE::Set_CrowdMemberVisible(Crowd, InMemberCurrent._MemberIndex, false);
            DoWrite_MemberFade(Crowd, InMemberCurrent._MemberIndex, FadeSlot, 1.0f);
            InMemberCurrent._FadeAlpha = 1.0f;
            InMemberCurrent._FadePhase = EVisualLod_FadePhase::None;

            // The steady near state is exactly 0, not merely the last sub-step: _FadeAlpha was just
            // reset to 1.0 for the FAR slot's clean parking value and no longer speaks for the proxy
            DoWrite_ProxyFade(InMemberCurrent, NearSlot, 0.0f);
        }
        else if (InMemberCurrent._FadePhase == EVisualLod_FadePhase::DemoteFade && InMemberCurrent._FadeAlpha >= 1.0f)
        {
            // Member is solid again — drop the proxy; the member is now the sole representation
            DoDemote_Finish(InCtx, InMember, InMemberCurrent);
        }
    }

    auto
        FProcessor_VisualLodArbiter_Update::
        DoUpdate_FarMember(
            FUpdateContext& InCtx,
            FCk_Handle_VisualLod InMember,
            FFragment_VisualLod_Current& InMemberCurrent,
            const FTransform& InMemberXf)
        -> void
    {
        const auto Crowd = InMemberCurrent._Crowd.Get();
        if (ck::Is_NOT_Valid(Crowd) || InMemberCurrent._MemberIndex == INDEX_NONE)
        { return; }

        UCk_Utils_IskmBatched_UE::Set_CrowdMemberTransform(Crowd, InMemberCurrent._MemberIndex, InMemberXf);

        const auto& CrowdConfig = InCtx._Config->Get_CrowdConfigs()[InMember.Get<FFragment_VisualLod_Params>().Get_CrowdIndex()];
        const auto [Seq, Rate] = DoCompute_FarAnim(InMemberCurrent._FarAnim, CrowdConfig, DoGet_PlanarSpeed(InMember));

        const auto SeqChanged  = Seq != InMemberCurrent._CurrentSequenceIndex;
        const auto RateChanged = FMath::Abs(Rate - InMemberCurrent._CurrentRate) > 0.1f;
        if (SeqChanged || RateChanged)
        {
            UCk_Utils_IskmBatched_UE::Set_CrowdMemberAnimation(Crowd, InMemberCurrent._MemberIndex, Seq, Rate, false);
            InMemberCurrent._CurrentSequenceIndex = Seq;
            InMemberCurrent._CurrentRate          = Rate;
        }
    }

    auto
        FProcessor_VisualLodArbiter_Update::
        DoDrive_ProxyAnim(
            FUpdateContext& InCtx,
            FCk_Handle_VisualLod InMember,
            FFragment_VisualLod_Current& InMemberCurrent)
        -> void
    {
        auto Proxy = InMemberCurrent._Proxy;
        if (ck::Is_NOT_Valid(Proxy))
        { return; }

        const auto CrowdIndex = InMember.Get<FFragment_VisualLod_Params>().Get_CrowdIndex();
        if (NOT InCtx._Current->_Crowds.IsValidIndex(CrowdIndex))
        { return; }

        // Only a crowd-backed member has a shared anim collection to mirror. AlwaysPromoted /
        // exhaustion-fallback proxies have no far representation — their animation is the game's
        const auto Collection = InCtx._Current->_Crowds[CrowdIndex]._Collection.Get();
        if (ck::Is_NOT_Valid(Collection))
        { return; }

        const auto& CrowdConfig = InCtx._Config->Get_CrowdConfigs()[CrowdIndex];
        const auto [Seq, Rate] = DoCompute_FarAnim(InMemberCurrent._FarAnim, CrowdConfig, DoGet_PlanarSpeed(InMember));

        const auto SeqChanged  = Seq != InMemberCurrent._ProxySequenceIndex;
        const auto RateChanged = FMath::Abs(Rate - InMemberCurrent._ProxyRate) > 0.1f;
        if (NOT SeqChanged && NOT RateChanged)
        { return; }

        const auto SeqInRange = Collection->Get_Sequences().IsValidIndex(Seq);
        CK_ENSURE_IF_NOT(SeqInRange,
            TEXT("VisualLod far-anim sequence index [{}] is outside crowd [{}]'s anim collection ([{}] sequences) — proxy [{}] keeps its current pose"),
            Seq, CrowdIndex, Collection->Get_Sequences().Num(), InMember)
        { return; }

        UAnimSequenceBase* const SeqAsset = Collection->Get_Sequences()[Seq].Get_Sequence();
        if (ck::Is_NOT_Valid(SeqAsset))
        { return; }

        // The crowd member's clock is THE clock (it advances even while hidden, at the member's
        // rate), so the proxy starts mid-sequence wherever the crowd currently is — the two
        // representations crossfade over each other, and a phase offset between them is the one
        // seam the overlapped dither masks cannot hide
        auto StartAt = 0.0f;
        const auto Crowd = InMemberCurrent._Crowd.Get();
        const auto HasCrowdSlot = ck::IsValid(Crowd) && InMemberCurrent._MemberIndex != INDEX_NONE;
        if (HasCrowdSlot)
        {
            const auto SeqLength = SeqAsset->GetPlayLength();
            if (SeqLength > 0.0f)
            {
                // Two systematic offsets are cancelled at the anchor — together they read as the
                // two bodies walking one frame apart: the play request applies NEXT frame, during
                // which the crowd's clock advances once more; and the crowd DISPLAYS
                // trunc(time * SampleFrequency), lagging its own clock by half a bake interval on
                // average. Both magnitudes are config-tunable — the mechanics say 1.0 and 0.5, the
                // game's eye gets the final word
                const auto Lead =
                    static_cast<float>(InCtx._DeltaT.Get_Seconds()) * Rate * InCtx._Config->Get_FadeAnchorLeadFrames()
                    - InCtx._Config->Get_FadeAnchorBakeLagIntervals()
                        / static_cast<float>(FMath::Max(1, Collection->Get_SampleFrequency()));

                StartAt = FMath::Fmod(UCk_Utils_IskmBatched_UE::Get_CrowdMemberAnimationTime(
                    Crowd, InMemberCurrent._MemberIndex) + Lead, SeqLength);
                if (StartAt < 0.0f)
                { StartAt += SeqLength; }
            }
        }

        // Sequence pose mode (drop any RendererData AnimBP) so PlayAnimation is not ignored, then
        // play the far sequence looping at the far rate. _Unique makes a same-sequence re-issue a
        // no-op, so only rate changes actually re-drive
        UCk_Utils_IskmProxy_UE::Request_SetAnimInstanceClass(Proxy, nullptr, FCk_Delegate_Request_OnCompleted{});

        const auto PlayRequest = FCk_Request_IskmProxy_PlayAnimation{TSoftObjectPtr<UAnimSequenceBase>{SeqAsset}}
            .Set_Loop(true)
            .Set_PlayRate(Rate)
            .Set_StartAt(StartAt)
            .Set_Unique(true);
        UCk_Utils_IskmProxy_UE::Request_PlayAnimation(Proxy, PlayRequest, FCk_Delegate_Request_OnCompleted{});

        InMemberCurrent._ProxySequenceIndex = Seq;
        InMemberCurrent._ProxyRate          = Rate;

        // Keep the hidden member's seq/rate current too, or its clock advances at a stale rate
        // while promoted and the phases drift apart — surfacing exactly at the next crossfade
        if (HasCrowdSlot)
        {
            UCk_Utils_IskmBatched_UE::Set_CrowdMemberAnimation(Crowd, InMemberCurrent._MemberIndex,
                Seq, Rate, false);
            InMemberCurrent._CurrentSequenceIndex = Seq;
            InMemberCurrent._CurrentRate          = Rate;
        }
    }

    auto
        FProcessor_VisualLodArbiter_Update::
        DoWrite_ProxyFade(
            const FFragment_VisualLod_Current& InMemberCurrent,
            int32 InNearSlot,
            float InAlpha)
        -> void
    {
        auto Proxy = InMemberCurrent.Get_Proxy();
        if (ck::Is_NOT_Valid(Proxy))
        { return; }

        // The custom-data lane rather than a direct SKMC write: it mirrors the value to every
        // attached submesh (the whole body fades as one), caches it across SKMC re-acquisition,
        // and its Setup zero-fill resets recycled proxies. The renderer data must declare
        // _NumCustomDataFloat > InNearSlot or the handler ensures loudly — that IS the contract.
        // The LATE lane specifically: the normal lane's handler has already run this frame (it
        // precedes this arbiter update), so it would display each alpha one frame behind the
        // crowd's slot and skew the two complementary masks against each other mid-fade
        UCk_Utils_IskmProxy_UE::Request_SetCustomDataFloat_Late(Proxy,
            FCk_Request_IskmProxy_SetCustomDataFloat{InNearSlot, InAlpha},
            FCk_Delegate_Request_OnCompleted{});
    }

    auto
        FProcessor_VisualLodArbiter_Update::
        DoCompute_FarAnim(
            const FCk_VisualLod_FarAnim& InFarAnim,
            const FCk_VisualLod_CrowdConfig& InCrowdConfig,
            float InPlanarSpeed)
        -> TTuple<int32, float>
    {
        if (InFarAnim.Get_Mode() == ECk_VisualLod_FarAnimMode::Fixed)
        { return MakeTuple(InFarAnim.Get_FixedSequenceIndex(), InFarAnim.Get_FixedRate()); }

        if (InPlanarSpeed > InCrowdConfig.Get_MoveSpeedThreshold())
        {
            const auto Rate = FMath::Clamp(InPlanarSpeed / InCrowdConfig.Get_MoveAuthoredSpeed(),
                static_cast<float>(InCrowdConfig.Get_MoveRateClamp().Get_Min()),
                static_cast<float>(InCrowdConfig.Get_MoveRateClamp().Get_Max()));
            return MakeTuple(InCrowdConfig.Get_MoveSequenceIndex(), Rate);
        }

        return MakeTuple(InCrowdConfig.Get_IdleSequenceIndex(), 1.0f);
    }

    auto
        FProcessor_VisualLodArbiter_Update::
        DoGet_PlanarSpeed(
            FCk_Handle_VisualLod InMember)
        -> float
    {
        if (NOT UCk_Utils_Velocity_UE::Has(InMember))
        { return 0.0f; }

        const auto Velocity = UCk_Utils_Velocity_UE::Get_CurrentVelocity(
            UCk_Utils_Velocity_UE::CastChecked(InMember));
        return static_cast<float>(FVector{Velocity.X, Velocity.Y, 0.0f}.Size());
    }

    auto
        FProcessor_VisualLodArbiter_Update::
        DoWrite_MemberFade(
            ACk_Iskm_BatchedCrowd_Actor* InCrowd,
            int32 InMemberIndex,
            int32 InFadeSlot,
            float InAlpha)
        -> void
    {
        UCk_Utils_IskmBatched_UE::Set_CrowdMemberCustomDataFloats(
            InCrowd, InMemberIndex, InFadeSlot, {InAlpha});
    }

    auto
        FProcessor_VisualLodArbiter_Update::
        DoRefund_Charge(
            FFragment_VisualLodArbiter_Current& InArbiterCurrent,
            FCk_Handle_VisualLod InMember,
            const FFragment_VisualLod_Current& InMemberCurrent)
        -> void
    {
        if (InMemberCurrent._PromotedViaLock)
        { InArbiterCurrent._LockedPromotedCount = FMath::Max(InArbiterCurrent._LockedPromotedCount - 1, 0); }
        else if (InMemberCurrent._PromotedUnbudgeted)
        { InArbiterCurrent._UnbudgetedPromotedCount = FMath::Max(InArbiterCurrent._UnbudgetedPromotedCount - 1, 0); }
        else
        { InArbiterCurrent._NearPromotedCount = FMath::Max(InArbiterCurrent._NearPromotedCount - 1, 0); }

        InArbiterCurrent._PromotedOwners.RemoveAll(
            [&](const FCk_Handle& InOwner) { return InOwner == InMember; });
    }

    auto
        FProcessor_VisualLodArbiter_Update::
        DoSweep_Step(
            FUpdateContext& InCtx)
        -> void
    {
        auto& Current = *InCtx._Current;

        // Reconciliation net behind the deterministic EndPlay release: a slot whose owner handle
        // died without the EndPlay path (e.g. arbiter re-assignment races) is returned here
        for (auto& Runtime : Current._Crowds)
        {
            const auto Crowd = Runtime._Crowd.Get();
            if (ck::Is_NOT_Valid(Crowd) || Runtime._SlotOwners.IsEmpty())
            { continue; }

            const auto Idx = Current._SweepCursor % Runtime._SlotOwners.Num();
            if (ck::IsValid(Runtime._SlotOwners[Idx]))
            { continue; }

            if (Runtime._FreeSlots.Contains(Idx))
            { continue; }

            UCk_Utils_IskmBatched_UE::Set_CrowdMemberVisible(Crowd, Idx, false);
            UCk_Utils_IskmBatched_UE::Clear_CrowdMemberCosmetics(Crowd, Idx);
            Runtime._FreeSlots.Add(Idx);
        }
        ++Current._SweepCursor;

        if (Current._PromotedOwners.Num() > 0)
        {
            const auto PromotedIdx = Current._SweepCursor % Current._PromotedOwners.Num();
            if (ck::Is_NOT_Valid(Current._PromotedOwners[PromotedIdx]))
            { Current._PromotedOwners.RemoveAt(PromotedIdx); }
        }
    }

    auto
        FProcessor_VisualLodArbiter_Update::
        DoRecycle_Slot(
            FFragment_VisualLodArbiter_Current& InArbiterCurrent,
            int32 InCrowdIndex,
            FCk_Handle_VisualLod InMember,
            int32 InMemberIndex)
        -> void
    {
        if (NOT InArbiterCurrent._Crowds.IsValidIndex(InCrowdIndex))
        { return; }

        auto& Runtime = InArbiterCurrent._Crowds[InCrowdIndex];

        // The stale-crowd block clears MemberIndex whenever the recorded crowd is gone or
        // replaced, so by here a held index always addresses the LIVE pool. Out of range means
        // the fragment and the pool disagree — loud, not silently skipped
        CK_ENSURE_IF_NOT(InMemberIndex >= 0 && InMemberIndex < Runtime._SlotOwners.Num(),
            TEXT("VisualLod crowd slot [{}] is outside the live pool of [{}] — not recycling"),
            InMemberIndex, Runtime._SlotOwners.Num())
        { return; }

        // Only the recorded owner may return a slot. Freeing someone else's index would hand a
        // live member to a second entity and render two bodies at one transform
        CK_ENSURE_IF_NOT(Runtime._SlotOwners[InMemberIndex] == InMember,
            TEXT("VisualLod crowd slot [{}] is owned by a different entity — not recycling"), InMemberIndex)
        { return; }

        Runtime._SlotOwners[InMemberIndex] = FCk_Handle{};
        if (NOT Runtime._FreeSlots.Contains(InMemberIndex))
        { Runtime._FreeSlots.Add(InMemberIndex); }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VisualLodArbiter_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_VisualLodArbiter_Requests& InRequests)
        -> void
    {
        request::FireCancelledForPending(InHandle, InRequests.Get_Requests());
    }
}

// --------------------------------------------------------------------------------------------------------------------
