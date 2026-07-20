#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkRecord/Record/CkRecord_Fragment.h"

#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Signal/CkSignal_Utils.h"
#include "CkEcs/Signal/CkSignal_Fragment.h"

#include "CkMinimap/CkMinimap_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Minimap_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_Minimap_NeedsSetup);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_Minimap_Params = FCk_Fragment_Minimap_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKMINIMAP_API FFragment_Minimap_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Minimap_Current);

    public:
        friend class FProcessor_Minimap_Setup;
        friend class FProcessor_Minimap_HandleRequests;
        friend class FProcessor_Minimap_Update;
        friend class FProcessor_Minimap_EndPlay;
        friend class ::UCk_Utils_Minimap_UE;

    private:
        // The entity whose Transform (position) and Camera/Transform (view yaw) drive the projection.
        // Seeded to the minimap child's LIFETIME OWNER at Setup; replaceable via Request_SetObserver.
        FCk_Handle _Observer;

        // Invalid = no fog culling. Set via Request_SetFogOfWar.
        FCk_Handle_FogOfWar _FogOfWar;

        // Live zoom (world cm center -> frame edge). Seeded from Params, mutable via Request_SetViewExtent.
        float _ViewExtent = 0.0f;

        // Seeded from Params, mutable via Request_SetRotationMode.
        ECk_Minimap_RotationMode _RotationMode = ECk_Minimap_RotationMode::NorthLocked;

        // Observer position/yaw as of the LAST projection update — the pull surface for widgets. Goes
        // stale WHOLESALE between throttled updates (there is no compass-style unthrottled channel).
        FVector _ViewOrigin = FVector::ZeroVector;
        float _ViewYawDegrees = 0.0f;

        // Sorted priority-desc then distance-asc; a self-contained snapshot per entry (see FCk_Minimap_Entry)
        TArray<FCk_Minimap_Entry> _Entries;

        // Reused build target for the next projection pass — swap-published into _Entries (no per-frame allocation churn)
        TArray<FCk_Minimap_Entry> _ScratchEntries;

        // Parallel-projection scratch (reused across updates): the POI set gathered from the view, then one
        // slot per POI written by the ParallelFor workers — disjoint index writes, no contention
        TArray<FCk_Entity> _ScratchPoiEntities;
        TArray<TOptional<FCk_Minimap_Entry>> _ScratchParallelSlots;

        float _TimeSinceUpdate = 0.0f;

    public:
        CK_PROPERTY_GET(_Observer);
        CK_PROPERTY_GET(_FogOfWar);
        CK_PROPERTY_GET(_ViewExtent);
        CK_PROPERTY_GET(_RotationMode);
        CK_PROPERTY_GET(_ViewOrigin);
        CK_PROPERTY_GET(_ViewYawDegrees);
        CK_PROPERTY_GET(_Entries);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKMINIMAP_API FFragment_Minimap_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Minimap_Requests);

    public:
        friend class FProcessor_Minimap_HandleRequests;
        friend class ::UCk_Utils_Minimap_UE;

    public:
        using RequestType = std::variant<FCk_Request_Minimap_SetViewExtent, FCk_Request_Minimap_SetCategoryFilter,
            FCk_Request_Minimap_SetObserver, FCk_Request_Minimap_SetRotationMode, FCk_Request_Minimap_SetFogOfWar>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // TRANSIENT: minimap children are derived per-client VIEW state — never persisted, never rebuilt on load
    // (a consumer re-Adds its maps when its HUD spins up). The policy suffix is an intent label post-purge.
    CK_DEFINE_RECORD_OF_ENTITIES_TRANSIENT(FFragment_RecordOfMinimaps, FCk_Handle_Minimap);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKMINIMAP_API, OnMinimapEntryAppeared, FCk_Delegate_Minimap_EntryAppeared, FCk_Handle_Minimap, FCk_Minimap_Entry);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKMINIMAP_API, OnMinimapEntryDisappeared, FCk_Delegate_Minimap_EntryDisappeared, FCk_Handle_Minimap, FCk_Handle_Poi);

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_Minimap_Requests);
}

// --------------------------------------------------------------------------------------------------------------------
