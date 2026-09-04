#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"
#include "CkEcs/Signal/CkSignal_Fragment.h"
#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Signal/CkSignal_Utils.h"
#include "CkEcs/Tag/CkTag.h"

#include "CkGroundNav/Field/CkGroundNav_Field.h"
#include "CkGroundNav/Path/CkGroundNavPath_Fragment_Data.h"
#include "CkGroundNav/Search/CkGroundNav_PathSearch.h"
#include "CkGroundNav/Search/CkGroundNav_PlatePortalGraph.h"

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_GroundNavPath_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    /** A search this agent has begun or is waiting on ground for. Drives the slice processor's view. */
    CK_DEFINE_ECS_TAG(FTag_GroundNavPath_SearchInFlight);

    /** Raised where a published surface rebuild's bounds meet this agent's cached corridor: the route
     *  it is walking crosses ground that moved. RAISED HERE AND NEVER REMOVED HERE - the path's consumer
     *  (the crowd's path refresh) is what clears it by acting on it, so a consumer that has not run yet
     *  finds the news still standing rather than losing it to whoever raised it. */
    CK_DEFINE_ECS_TAG(FTag_GroundNavPath_RepathRequired);

    // ----------------------------------------------------------------------------------------------------------------

    using FFragment_GroundNavPath_Params = FCk_Fragment_GroundNavPath_ParamsData;

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * The episode in flight: the search itself, the field it is reading, and the request that asked
     * for it.
     *
     * MUST NEVER BE REPLICATED OR SNAPSHOTTED. Three separate reasons, each on its own sufficient:
     * the search holds a TSharedPtr into a field that exists only in this process, _PendingSince is a
     * PROCESS-relative absolute timestamp that reads as ancient anywhere else and would trip the
     * deferral timeout instantly, and the corridor keys are only meaningful against the field epoch
     * they were found on. If this fragment ever gains a persistence or replication handler, it has to
     * be excluded whole - there is nothing here to rebase.
     */
    struct CKGROUNDNAV_API FFragment_GroundNavPath_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_GroundNavPath_Current);

        friend class FProcessor_GroundNavPath_HandleRequests;
        friend class FProcessor_GroundNavPath_Slice;
        friend class FProcessor_GroundNavPath_CancelPendingRequests;
        friend class ::UCk_Utils_GroundNavPath_UE;

        // The episode transitions both processors share - begin, publish, clear. Defined in the
        // processor TU; a struct rather than a free function only because a namespace cannot be a friend.
        friend struct FGroundNavPath_Episode;

    private:
        groundnav::FCk_GroundNav_PathSearch _Search;

        // The snapshot the search began on, held for the whole episode: a rebuild swaps the registry's
        // pointer and this copy is what keeps the structure being walked whole - and it is what the
        // post-process is run against, so the plan is funnelled over the field the corridor was found on.
        groundnav::FCk_GroundNav_FieldPtr _Field;

        /** The request riding the whole multi-frame episode. The drain that accepted it cannot report
         *  the outcome, because the outcome is frames away; and a deferred episode re-reads its own
         *  From and Goal off this every time it retries. */
        FCk_Request_GroundNavPath_FindPath _PendingRequest;

        // False while the episode is parked on unbuilt ground: no search has been stood up yet.
        bool _HasBegun = false;

        // FPlatformTime::Seconds() when this episode was parked. See the replication note above.
        FCk_Time _PendingSince;

        // What this episode's slices have spent searching. Held apart from _PendingSince because the
        // wait and the work answer different questions: one dates the episode, the other prices it.
        FCk_Time _SearchTimeSpent;

        /** The corridor of the last plan, keyed by the ONE durable identity a crossing has. Node ids
         *  are per-search pool ids and mean nothing to a second search, so a stored corridor is stored
         *  as keys or it is not stored at all. Kept for a later repair to re-canonicalise against. */
        TArray<groundnav::FCk_GroundNav_CrossingKey> _LastCorridorKeys;

        /** The AUTHORED ids of the links the corridor above crosses, in walk order and without
         *  repeats, resolved against the field the plan was made on. The key's own _LinkIndex cannot
         *  serve: _ResolvedLinks is rebuilt wholesale per publish, so one removal renumbers every entry
         *  after it and an index cached here would name a different link on the next field. An id is
         *  volume-scoped, monotone and never reused, which is what lets an invalidator ask a LATER
         *  publish whether it moved anything this route depends on. Empty for a route that crosses none. */
        TArray<int32> _LastCorridorLinkIds;

        // The flat plate the last plan started from. A repair may only warm-start from a corridor whose
        // source the body still stands on.
        int32 _LastSourceFlatPlate = INDEX_NONE;

        /** The field epoch the keys above were found on. Held WITH them because a key means nothing
         *  apart from its epoch: a repair compares the two before it trusts a door it did not re-walk,
         *  and an epoch read off the published result instead would date the last PUBLISH rather than
         *  the last corridor. */
        groundnav::FCk_GroundNav_Epoch _LastCorridorEpoch;

        /** The profile the corridor above was planned for, taken from the request the episode opened
         *  on. Held WITH the corridor for the same reason its epoch is: a corridor is a statement about
         *  ONE field, and resolving the volume's untagged default for a route planned over a variant
         *  would compare that plan's epoch against a field it never moved with. Empty is the untagged
         *  default, which is what an untagged request plans over. */
        FGameplayTag _ProfileTag;

        /** The world box the corridor's plates cover, stored ALREADY inflated by _CorridorInflationUu.
         *  An invalidator asks it on every republish to decide whether a rebuilt tile could have moved
         *  this route, so it must not have to re-walk the corridor or re-derive a margin of its own.
         *  Invalid where nothing is cached. */
        FBox _LastCorridorBounds = FBox{ForceInit};

        // What the box above was inflated by - the body's radius plus the corridor margin. Beside the
        // box rather than derivable from it, so a reader recovers the plates' own union exactly.
        float _CorridorInflationUu = 0.0f;

    public:
        CK_PROPERTY_GET(_Field);
        CK_PROPERTY_GET(_HasBegun);
        CK_PROPERTY_GET(_PendingSince);
        CK_PROPERTY_GET(_LastCorridorKeys);
        CK_PROPERTY_GET(_LastCorridorLinkIds);
        CK_PROPERTY_GET(_LastSourceFlatPlate);
        CK_PROPERTY_GET(_LastCorridorEpoch);
        CK_PROPERTY_GET(_ProfileTag);
        CK_PROPERTY_GET(_LastCorridorBounds);
        CK_PROPERTY_GET(_CorridorInflationUu);
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * What the last FINISHED episode answered.
     *
     * _HasFreshResult is what separates a slot nobody has answered yet from one carrying a terminal
     * verdict: the status alone cannot, because a failed episode and an episode that has not run both
     * leave a non-Ready status behind. It is set when a result is published and cleared the moment a
     * new episode is parked, so it is true exactly while the result belongs to no in-flight search.
     */
    struct CKGROUNDNAV_API FFragment_GroundNavPath_Result
    {
    public:
        CK_GENERATED_BODY(FFragment_GroundNavPath_Result);

        friend class FProcessor_GroundNavPath_HandleRequests;
        friend class FProcessor_GroundNavPath_Slice;
        friend class ::UCk_Utils_GroundNavPath_UE;
        friend struct FGroundNavPath_Episode;

    private:
        FCk_GroundNavPath_Result _Result;

        bool _HasFreshResult = false;

    public:
        CK_PROPERTY_GET(_Result);
        CK_PROPERTY_GET(_HasFreshResult);
    };

    // ----------------------------------------------------------------------------------------------------------------

    struct CKGROUNDNAV_API FFragment_GroundNavPath_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_GroundNavPath_Requests);

        friend class FProcessor_GroundNavPath_HandleRequests;
        friend class ::UCk_Utils_GroundNavPath_UE;

    public:
        using RequestType = std::variant<
            FCk_Request_GroundNavPath_FindPath,
            FCk_Request_GroundNavPath_AbandonPath>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // ----------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKGROUNDNAV_API,
        OnGroundNavPathReady,
        FCk_Delegate_GroundNavPath_OnPathReady,
        FCk_Handle_GroundNavPath);

    // Carries WHICH terminal status, because a consumer defers on unbuilt ground, gives up on ground
    // with nowhere to stand, and retries with a wider budget on an exhausted one.
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKGROUNDNAV_API,
        OnGroundNavPathFailed,
        FCk_Delegate_GroundNavPath_OnPathFailed,
        FCk_Handle_GroundNavPath,
        ECk_GroundNav_PathStatus);

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_GroundNavPath_Requests);
}

// --------------------------------------------------------------------------------------------------------------------
