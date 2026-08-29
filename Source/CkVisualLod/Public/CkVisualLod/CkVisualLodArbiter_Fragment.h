#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Signal/CkSignal_Utils.h"
#include "CkEcs/Signal/CkSignal_Fragment.h"
#include "CkEcs/Tag/CkTag.h"

#include "CkResourceLoader/CkResourceLoader_Fragment_Data.h"

#include "CkVisualLod/CkVisualLodArbiter_Fragment_Data.h"
#include "CkVisualLod/CkVisualLod_Ranking.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_VisualLodArbiter_UE;
class ACk_Iskm_BatchedCrowd_Actor;
class UCk_IskmAnimCollection_Data;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_VisualLodArbiter_Setup;
    class FProcessor_VisualLodArbiter_HandleRequests;
    class FProcessor_VisualLodArbiter_Update;
    class FProcessor_VisualLod_EndPlay;

    CK_DEFINE_ECS_TAG(FTag_VisualLodArbiter_NeedsSetup);

    // Observability only: present while the arbiter's config asset batch is still loading
    CK_DEFINE_ECS_TAG(FTag_VisualLodArbiter_PendingAssetLoad);

    // Inspection hold (Request_SetFrozen): the arbiter takes no new LOD decisions while present —
    // no gather, rank, flips, preempts or far updates. In-flight fades still step to completion so
    // no member is stranded mid-crossfade, and external-teardown recovery still fails closed
    CK_DEFINE_ECS_TAG(FTag_VisualLodArbiter_Frozen);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_VisualLodArbiter_Params = FCk_Fragment_VisualLodArbiter_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    // One batched crowd's live pool. The owner arrays are the authority on slot ownership; the
    // fragment-side MemberIndex is reconciled against them (stale-crowd invalidation + sweep)
    struct CKVISUALLOD_API FVisualLod_CrowdRuntime
    {
    public:
        CK_GENERATED_BODY(FVisualLod_CrowdRuntime);

    public:
        TWeakObjectPtr<ACk_Iskm_BatchedCrowd_Actor> _Crowd;

        TArray<int32> _FreeSlots;

        TArray<FCk_Handle> _SlotOwners;

        // Roots the crowd's anim collection for the crowd actor's lifetime
        FCk_ResourceLoader_RootedAssetBatch _LoadedAssets;

        // Resolved anim collection (rooted by _LoadedAssets). Source for the promoted proxy's
        // single-sequence locomotion, so a near proxy mirrors the far member's animation without a
        // heavy AnimBP. Invalid until the crowd stands up
        TWeakObjectPtr<const UCk_IskmAnimCollection_Data> _Collection;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKVISUALLOD_API FFragment_VisualLodArbiter_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_VisualLodArbiter_Current);

    public:
        friend class FProcessor_VisualLodArbiter_Setup;
        friend class FProcessor_VisualLodArbiter_HandleRequests;
        friend class FProcessor_VisualLodArbiter_Update;
        friend class FProcessor_VisualLod_EndPlay;
        friend class ::UCk_Utils_VisualLodArbiter_UE;

    private:
        // Resolved config asset — rooted by _LoadedAssets below for the arbiter's lifetime
        TWeakObjectPtr<const UCk_VisualLodArbiter_Data> _Config;

        FCk_ResourceLoader_RootedAssetBatch _LoadedAssets;

        // Explicit observer (Request_SetObserver); invalid = fall back to local-view discovery
        FCk_Handle _Observer;

        // One runtime pool per config CrowdConfigs entry, index-aligned. Crowds stand up lazily
        // on the first entity that needs a slot
        TArray<FVisualLod_CrowdRuntime> _Crowds;

        // Entities currently holding a promoted proxy, and how each is charged. Near + Locked are
        // the two budgets; Unbudgeted (AlwaysPromoted / exhaustion-fallback) counts toward neither
        TArray<FCk_Handle> _PromotedOwners;
        int32 _NearPromotedCount = 0;
        int32 _LockedPromotedCount = 0;
        int32 _UnbudgetedPromotedCount = 0;

        int32 _SweepCursor = 0;

        // Rank input collected by the per-entity pass and consumed by the flip pass, kept as
        // members so the per-tick arrays aren't reallocated
        TArray<FVisualLod_RankEntry> _Candidates;
        TArray<FVisualLod_RankEntry> _Incumbents;

        // The view this arbiter resolved on its last update, retained so tooling reads the same
        // view the ranking used. Invalid = no observer and no local view that tick
        FVisualLod_LocalView _LastView;

        // Flip accounting for the update in progress; all three reset at the top of every arbiter
        // update, so a frozen or view-less arbiter reads zero. A preempt-demote counts in Preempts
        // only — it is a ranking outcome, not a distance demote
        int32 _PromotesThisTick = 0;
        int32 _DemotesThisTick = 0;
        int32 _PreemptsThisTick = 0;

    public:
        CK_PROPERTY_GET(_Config);
        CK_PROPERTY_GET(_Observer);
        CK_PROPERTY_GET(_PromotedOwners);
        CK_PROPERTY_GET(_NearPromotedCount);
        CK_PROPERTY_GET(_LockedPromotedCount);
        CK_PROPERTY_GET(_UnbudgetedPromotedCount);
        CK_PROPERTY_GET(_LastView);
        CK_PROPERTY_GET(_PromotesThisTick);
        CK_PROPERTY_GET(_DemotesThisTick);
        CK_PROPERTY_GET(_PreemptsThisTick);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKVISUALLOD_API FFragment_VisualLodArbiter_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_VisualLodArbiter_Requests);

    public:
        friend class FProcessor_VisualLodArbiter_HandleRequests;
        friend class ::UCk_Utils_VisualLodArbiter_UE;

    public:
        using RequestType = std::variant<
            FCk_Request_VisualLodArbiter_SetObserver,
            FCk_Request_VisualLodArbiter_ClearObserver,
            FCk_Request_VisualLodArbiter_SetFrozen>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKVISUALLOD_API, OnVisualLodArbiter_CrowdCreated, FCk_Delegate_VisualLodArbiter_CrowdCreated, FCk_Handle_VisualLodArbiter, int32);

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_VisualLodArbiter_Requests);
}

// --------------------------------------------------------------------------------------------------------------------
