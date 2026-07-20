#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Signal/CkSignal_Utils.h"
#include "CkEcs/Signal/CkSignal_Fragment.h"

#include "CkCompass/CkCompass_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Compass_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_Compass_NeedsSetup);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_Compass_Params = FCk_Fragment_Compass_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKCOMPASS_API FFragment_Compass_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Compass_Current);

    public:
        friend class FProcessor_Compass_Setup;
        friend class FProcessor_Compass_HandleRequests;
        friend class FProcessor_Compass_Update;
        friend class FProcessor_Compass_EndPlay;
        friend class ::UCk_Utils_Compass_UE;

    private:
        // The entity whose Transform (position) and Camera/Transform (view yaw) drive the projection.
        // Seeded to the compass' own entity at Setup; replaceable via Request_SetObserver.
        FCk_Handle _Observer;

        // Absolute world yaw, 0-360, refreshed EVERY frame (never throttled by the update interval)
        float _HeadingDegrees = 0.0f;

        // Consumed only when HeadingSource is Manual
        float _ManualHeadingDegrees = 0.0f;

        // Sorted priority-desc then distance-asc; a self-contained snapshot per entry (see FCk_Compass_Entry)
        TArray<FCk_Compass_Entry> _Entries;

        // Reused build target for the next projection pass — swap-published into _Entries (no per-frame allocation churn)
        TArray<FCk_Compass_Entry> _ScratchEntries;

        // Parallel-projection scratch (reused across updates): the POI set gathered from the view, then one
        // slot per POI written by the ParallelFor workers — disjoint index writes, no contention
        TArray<FCk_Entity> _ScratchPoiEntities;
        TArray<TOptional<FCk_Compass_Entry>> _ScratchParallelSlots;

        FCk_Time _TimeSinceUpdate;

    public:
        CK_PROPERTY_GET(_Observer);
        CK_PROPERTY_GET(_HeadingDegrees);
        CK_PROPERTY_GET(_Entries);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKCOMPASS_API FFragment_Compass_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Compass_Requests);

    public:
        friend class FProcessor_Compass_HandleRequests;
        friend class ::UCk_Utils_Compass_UE;

    public:
        using RequestType = std::variant<FCk_Request_Compass_SetCategoryFilter, FCk_Request_Compass_SetManualHeading, FCk_Request_Compass_SetObserver>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKCOMPASS_API, OnCompassEntryAppeared, FCk_Delegate_Compass_EntryAppeared, FCk_Handle_Compass, FCk_Compass_Entry);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKCOMPASS_API, OnCompassEntryDisappeared, FCk_Delegate_Compass_EntryDisappeared, FCk_Handle_Compass, FCk_Handle_Poi);

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_Compass_Requests);
}

// --------------------------------------------------------------------------------------------------------------------