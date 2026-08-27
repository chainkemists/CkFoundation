#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"
#include "CkEcs/Tag/CkTag.h"

#include "CkResourceLoader/CkResourceLoader_Fragment_Data.h"

#include "CkVisualLod/CkVisualLodArbiter_Fragment_Data.h"
#include "CkVisualLod/CkVisualLod_Ranking.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_VisualLodArbiter_UE;
class ACk_Iskm_BatchedCrowd_Actor;

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

    public:
        CK_PROPERTY_GET(_Config);
        CK_PROPERTY_GET(_Observer);
        CK_PROPERTY_GET(_PromotedOwners);
        CK_PROPERTY_GET(_NearPromotedCount);
        CK_PROPERTY_GET(_LockedPromotedCount);
        CK_PROPERTY_GET(_UnbudgetedPromotedCount);
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
            FCk_Request_VisualLodArbiter_ClearObserver>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_VisualLodArbiter_Requests);
}

// --------------------------------------------------------------------------------------------------------------------
