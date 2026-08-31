#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Signal/CkSignal_Utils.h"
#include "CkEcs/Signal/CkSignal_Fragment.h"
#include "CkEcs/Tag/CkTag.h"

#include "CkResourceLoader/CkResourceLoader_Fragment_Data.h"

#include "CkVisualLod/CkVisualLod_Fragment_Data.h"
#include "CkVisualLod/CkVisualLodArbiter_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_VisualLod_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_VisualLod_Setup;
    class FProcessor_VisualLod_HandleRequests;
    class FProcessor_VisualLod_EndPlay;
    class FProcessor_VisualLodArbiter_Update;

    CK_DEFINE_ECS_TAG(FTag_VisualLod_NeedsSetup);

    // Maintained by the arbiter's flip: present exactly while the entity holds a promoted proxy
    CK_DEFINE_ECS_TAG(FTag_VisualLod_Promoted);

    // External ownership (Request_Suspend): the arbiter does not touch this entity's
    // representation while present
    CK_DEFINE_ECS_TAG(FTag_VisualLod_Suspended);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_VisualLod_Params = FCk_Fragment_VisualLod_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    // Crossfade phase across the promote/demote flip. None = steady state
    enum class EVisualLod_FadePhase : uint8
    {
        None,
        PromoteFade,
        DemoteFade
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKVISUALLOD_API FFragment_VisualLod_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_VisualLod_Current);

    public:
        friend class FProcessor_VisualLod_Setup;
        friend class FProcessor_VisualLod_HandleRequests;
        friend class FProcessor_VisualLod_EndPlay;
        friend class FProcessor_VisualLodArbiter_Update;
        friend class ::UCk_Utils_VisualLod_UE;

    private:
        // Resolved from Params' arbiter tag (or Request_SetArbiter); invalid = unmanaged
        FCk_Handle_VisualLodArbiter _Arbiter;

        // Slot in the crowd's member pool; INDEX_NONE until acquired. Valid only against the exact
        // crowd recorded at acquisition — the arbiter invalidates stale ownership
        int32 _MemberIndex = INDEX_NONE;

        TWeakObjectPtr<ACk_Iskm_BatchedCrowd_Actor> _Crowd;

        bool _Promoted = false;

        // Which promote budget this entity was charged against, so the refund goes back to the
        // same one. Unbudgeted promotes (AlwaysPromoted / pool-exhaustion fallback) set neither
        bool _PromotedViaLock = false;
        bool _PromotedUnbudgeted = false;

        // Combat-visual promote lock — a counter, not a bool, so holders can overlap at a state
        // boundary without the release/acquire ordering mattering
        int32 _PromoteLock = 0;

        bool _Hidden = false;

        EVisualLod_FadePhase _FadePhase = EVisualLod_FadePhase::None;

        // Mirrors the member's fade custom-data float: 1 = member fully visible, 0 = dissolved
        float _FadeAlpha = 1.0f;

        // This demote was preemption, not distance. A preempted entity is inside the promote band
        // by construction, so the fade's near-side reversal would undo the demote next tick —
        // the flag holds the fade on its demote course. A promote lock still reverses and clears it
        bool _PreemptDemote = false;

        FCk_VisualLod_FarAnim _FarAnim;

        // Last sequence/rate pushed to the member, so the far update only calls into the crowd on
        // actual changes. Per-slot state: reset on slot release
        int32 _CurrentSequenceIndex = INDEX_NONE;
        float _CurrentRate = 1.0f;

        // Index into the crowd config's ordered RenderBands. INDEX_NONE means legacy (no bands)
        // or no current crowd slot. Retained while promoted so demotion can restore the correct
        // batched profile without changing ownership, budget, fade, or cosmetics.
        int32 _RenderBandIndex = INDEX_NONE;

        // Last sequence/rate pushed to the PROMOTED PROXY (distinct from the crowd-slot cache
        // above), so the promote drive only re-issues PlayAnimation on an actual change. Reset when
        // the proxy is torn down
        int32 _ProxySequenceIndex = INDEX_NONE;
        float _ProxyRate = 1.0f;

        // Scene-node child hosting the promoted proxy (valid only while promoted). Destroying it
        // routes the pooled SKMC release through the framework's own IskmProxy EndPlay
        FCk_Handle _VisualNode;

        FCk_Handle_IskmProxy _Proxy;

        // Runtime renderer override (Request_SetRenderer); Params' renderer when unset
        TSoftObjectPtr<UCk_IskmRenderer_Data> _RendererOverride;

        // Roots the resolved renderer data across a promote (see Source/CLAUDE.md rooted-batch rules)
        FCk_ResourceLoader_RootedAssetBatch _LoadedAssets;

        // The rank inputs the arbiter last computed for this member, retained so tooling can read
        // what the ranking actually saw. -1 distance = the arbiter has never ranked this member;
        // both go stale on any tick the arbiter skips it before the distance is computed
        float _LastDistance = -1.0f;
        bool _LastInView = false;

    public:
        CK_PROPERTY_GET(_Arbiter);
        CK_PROPERTY_GET(_MemberIndex);
        CK_PROPERTY_GET(_Crowd);
        CK_PROPERTY_GET(_Promoted);
        CK_PROPERTY_GET(_PromotedViaLock);
        CK_PROPERTY_GET(_PromotedUnbudgeted);
        CK_PROPERTY_GET(_PromoteLock);
        CK_PROPERTY_GET(_Hidden);
        CK_PROPERTY_GET(_FadePhase);
        CK_PROPERTY_GET(_FadeAlpha);
        CK_PROPERTY_GET(_PreemptDemote);
        CK_PROPERTY_GET(_CurrentSequenceIndex);
        CK_PROPERTY_GET(_CurrentRate);
        CK_PROPERTY_GET(_RenderBandIndex);
        CK_PROPERTY_GET(_ProxySequenceIndex);
        CK_PROPERTY_GET(_ProxyRate);
        CK_PROPERTY_GET(_Proxy);
        CK_PROPERTY_GET(_VisualNode);
        CK_PROPERTY_GET(_LastDistance);
        CK_PROPERTY_GET(_LastInView);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKVISUALLOD_API FFragment_VisualLod_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_VisualLod_Requests);

    public:
        friend class FProcessor_VisualLod_HandleRequests;
        friend class ::UCk_Utils_VisualLod_UE;

    public:
        using RequestType = std::variant<
            FCk_Request_VisualLod_SetArbiter,
            FCk_Request_VisualLod_SetVisibility,
            FCk_Request_VisualLod_SetFarAnim,
            FCk_Request_VisualLod_SetRenderer,
            FCk_Request_VisualLod_Suspend,
            FCk_Request_VisualLod_Resume>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKVISUALLOD_API, OnVisualLod_MemberAcquired, FCk_Delegate_VisualLod_MemberEvent, FCk_Handle_VisualLod, int32);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKVISUALLOD_API, OnVisualLod_Promoted, FCk_Delegate_VisualLod_Promoted, FCk_Handle_VisualLod, FCk_Handle_IskmProxy);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKVISUALLOD_API, OnVisualLod_DemoteFinishing, FCk_Delegate_VisualLod_MemberEvent, FCk_Handle_VisualLod, int32);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKVISUALLOD_API, OnVisualLod_MemberReleased, FCk_Delegate_VisualLod_MemberEvent, FCk_Handle_VisualLod, int32);

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_VisualLod_Requests);
}

// --------------------------------------------------------------------------------------------------------------------
