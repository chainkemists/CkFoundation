#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkVisualLod/CkVisualLod_Fragment.h"
#include "CkVisualLod/CkVisualLodArbiter_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKVISUALLOD_API FProcessor_VisualLodArbiter_Setup
        : public ck_exp::TProcessor<FProcessor_VisualLodArbiter_Setup, FCk_Handle_VisualLodArbiter,
            ck::TReadOnly<FFragment_VisualLodArbiter_Params>, ck::TReadWrite<FFragment_VisualLodArbiter_Current>,
            FTag_VisualLodArbiter_NeedsSetup, CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using MarkedDirtyBy = FTag_VisualLodArbiter_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_VisualLodArbiter_Params& InParams,
            FFragment_VisualLodArbiter_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKVISUALLOD_API FProcessor_VisualLodArbiter_HandleRequests
        : public ck_exp::TProcessor<FProcessor_VisualLodArbiter_HandleRequests, FCk_Handle_VisualLodArbiter,
            ck::TReadWrite<FFragment_VisualLodArbiter_Current>, ck::TReadWrite<FFragment_VisualLodArbiter_Requests>,
            TExclude<FTag_VisualLodArbiter_NeedsSetup>, TExclude<FTag_DestroyEntity_Initiate>, CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using RunAfter = TDepList<FProcessor_VisualLodArbiter_Setup>;
        using MarkedDirtyBy = FFragment_VisualLodArbiter_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_VisualLodArbiter_Current& InCurrent,
            FFragment_VisualLodArbiter_Requests& InRequests) const
            -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VisualLodArbiter_Current& InCurrent,
            const FCk_Request_VisualLodArbiter_SetObserver& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VisualLodArbiter_Current& InCurrent,
            const FCk_Request_VisualLodArbiter_ClearObserver& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VisualLodArbiter_Current& InCurrent,
            const FCk_Request_VisualLodArbiter_SetFrozen& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // The flip driver: resolves the view, walks the domain's members (per-entity flips + far
    // updates), then spends the promote budgets on the ranked best. FGroup_Gameplay_Script is the
    // slot the BB flip drivers occupied: after this frame's gameplay, and before
    // FProcessor_IskmCrowd_Advance (FGroup_Transform_SyncFrom) reads the member-world writes.
    //
    // DoTick is shadowed (not the generated view iteration): the promote path CREATES entities
    // (scene-node children), which is illegal inside a live view — so arbiters and members are
    // snapshotted into scratch arrays first and the whole mechanism runs outside any iteration.
    // The shadow writes _LastVisitedCount so the pump accounting stays truthful
    class CKVISUALLOD_API FProcessor_VisualLodArbiter_Update
        : public ck_exp::TProcessor<FProcessor_VisualLodArbiter_Update, FCk_Handle_VisualLodArbiter,
            ck::TReadOnly<FFragment_VisualLodArbiter_Params>, ck::TReadWrite<FFragment_VisualLodArbiter_Current>,
            TExclude<FTag_VisualLodArbiter_NeedsSetup>, CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Script;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

        // Never dispatched — the DoTick shadow above replaces the generated view iteration; this
        // exists only to satisfy the ck_exp::TProcessor CRTP contract, and ensures loudly if the
        // shadow is ever removed
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_VisualLodArbiter_Params& InParams,
            FFragment_VisualLodArbiter_Current& InCurrent)
            -> void;

    private:
        enum class EChargeClass : uint8
        {
            Near,
            Locked,
            Unbudgeted
        };

        // Per-arbiter per-tick working set; fragment pointers are stable for the tick
        // (in_place_delete pools)
        struct FUpdateContext
        {
            FCk_Time _DeltaT = FCk_Time::ZeroSecond();
            FCk_Handle_VisualLodArbiter _Arbiter;
            FFragment_VisualLodArbiter_Current* _Current = nullptr;
            const UCk_VisualLodArbiter_Data* _Config = nullptr;
            FVisualLod_LocalView _View;
            UWorld* _World = nullptr;
            TArray<FCk_Handle_VisualLod> _Members;
        };

    private:
        static auto
        DoUpdate_Arbiter(
            FCk_Time InDeltaT,
            FCk_Handle_VisualLodArbiter& InArbiter) -> int32;

        static auto
        DoResolve_View(
            const FCk_Handle_VisualLodArbiter& InArbiter,
            const FFragment_VisualLodArbiter_Current& InCurrent,
            const UCk_VisualLodArbiter_Data& InConfig) -> FVisualLod_LocalView;

        static auto
        DoGather_Members(
            FUpdateContext& InCtx) -> void;

        // The frozen arbiter's whole update: revisit the members it ALREADY owns (a frozen arbiter
        // claims no new ones by tag) to finish in-flight fades and fail closed on externally torn
        // down proxies. No gather-by-tag, ranking, flips, preempts or steady-state far updates
        static auto
        DoUpdate_ArbiterFrozen(
            FUpdateContext& InCtx) -> int32;

        static auto
        DoProcess_Member(
            FUpdateContext& InCtx,
            int32 InScratchIdx,
            FCk_Handle_VisualLod InMember) -> void;

        static auto
        DoProcess_Member_Frozen(
            FUpdateContext& InCtx,
            FCk_Handle_VisualLod InMember) -> void;

        static auto
        DoApply_RankedFlips(
            FUpdateContext& InCtx) -> void;

        static auto
        DoEnsure_Crowd(
            FUpdateContext& InCtx,
            int32 InCrowdIndex) -> ACk_Iskm_BatchedCrowd_Actor*;

        static auto
        DoTryAcquire_Member(
            FUpdateContext& InCtx,
            FCk_Handle_VisualLod InMember,
            FFragment_VisualLod_Current& InMemberCurrent,
            const FTransform& InMemberXf) -> void;

        static auto
        DoRelease_Member(
            FUpdateContext& InCtx,
            FCk_Handle_VisualLod InMember,
            FFragment_VisualLod_Current& InMemberCurrent) -> void;

        static auto
        DoPromote(
            FUpdateContext& InCtx,
            FCk_Handle_VisualLod InMember,
            FFragment_VisualLod_Current& InMemberCurrent,
            const FTransform& InMemberXf,
            EChargeClass InChargeClass) -> void;

        static auto
        DoDemote_Begin(
            FUpdateContext& InCtx,
            FCk_Handle_VisualLod InMember,
            FFragment_VisualLod_Current& InMemberCurrent,
            const FTransform& InMemberXf) -> void;

        static auto
        DoDemote_Finish(
            FUpdateContext& InCtx,
            FCk_Handle_VisualLod InMember,
            FFragment_VisualLod_Current& InMemberCurrent) -> void;

        static auto
        DoRecover_FailClosed(
            FUpdateContext& InCtx,
            FCk_Handle_VisualLod InMember,
            FFragment_VisualLod_Current& InMemberCurrent) -> void;

        // InAllowReversal gates the band/lock re-aim that lets a fade turn around mid-flight. That
        // re-aim IS a promote/demote decision, so a frozen arbiter passes false and the fade runs
        // out on the course it started
        static auto
        DoTick_Fade(
            FUpdateContext& InCtx,
            FCk_Handle_VisualLod InMember,
            FFragment_VisualLod_Current& InMemberCurrent,
            const FTransform& InMemberXf,
            float InDistance,
            bool InLockHeld,
            bool InAllowReversal) -> void;

        static auto
        DoUpdate_FarMember(
            FUpdateContext& InCtx,
            FCk_Handle_VisualLod InMember,
            FFragment_VisualLod_Current& InMemberCurrent,
            const FTransform& InMemberXf) -> void;

        // Mirrors the member's resolved far-anim onto the promoted proxy as a single looping
        // sequence (sequence pose mode, no AnimBP) so the near proxy keeps walking exactly as the
        // far member did. Idempotent per (seq, rate) via the proxy cache; a game wanting a richer
        // AnimBP overrides it in its OnVisualLod_Promoted handler (its request wins by FIFO order).
        // No-op when the member holds no crowd collection (AlwaysPromoted / exhaustion-fallback)
        static auto
        DoDrive_ProxyAnim(
            FUpdateContext& InCtx,
            FCk_Handle_VisualLod InMember,
            FFragment_VisualLod_Current& InMemberCurrent) -> void;

        static auto
        DoCompute_FarAnim(
            const FCk_VisualLod_FarAnim& InFarAnim,
            const FCk_VisualLod_CrowdConfig& InCrowdConfig,
            float InPlanarSpeed) -> TTuple<int32, float>;

        static auto
        DoGet_PlanarSpeed(
            FCk_Handle_VisualLod InMember) -> float;

        static auto
        DoWrite_MemberFade(
            ACk_Iskm_BatchedCrowd_Actor* InCrowd,
            int32 InMemberIndex,
            int32 InFadeSlot,
            float InAlpha) -> void;

        static auto
        DoSweep_Step(
            FUpdateContext& InCtx) -> void;

        // Near-side twin of DoWrite_MemberFade: the promoted proxy's mesh carries the SAME alpha on
        // its custom primitive data (submesh-mirrored via the proxy's custom-data lane), and its
        // material dithers itself OUT as that value rises — so the near and far masks are complements.
        // No release-path reset needed: proxy Setup zeroes declared slots on every (re)acquire
        static auto
        DoWrite_ProxyFade(
            const FFragment_VisualLod_Current& InMemberCurrent,
            int32 InNearSlot,
            float InAlpha) -> void;

    public:
        // Shared with FProcessor_VisualLod_EndPlay (deterministic death-path release + refund)
        static auto
        DoRecycle_Slot(
            FFragment_VisualLodArbiter_Current& InArbiterCurrent,
            int32 InCrowdIndex,
            FCk_Handle_VisualLod InMember,
            int32 InMemberIndex) -> void;

        static auto
        DoRefund_Charge(
            FFragment_VisualLodArbiter_Current& InArbiterCurrent,
            FCk_Handle_VisualLod InMember,
            const FFragment_VisualLod_Current& InMemberCurrent) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKVISUALLOD_API FProcessor_VisualLodArbiter_CancelPendingRequests
        : public ck_exp::TProcessor<FProcessor_VisualLodArbiter_CancelPendingRequests, FCk_Handle_VisualLodArbiter,
            ck::TReadOnly<FFragment_VisualLodArbiter_Requests>, CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_VisualLodArbiter_Requests& InRequests)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
