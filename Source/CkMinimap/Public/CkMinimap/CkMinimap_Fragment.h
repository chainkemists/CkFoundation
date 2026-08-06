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

    // The retained immutable residue of FCk_Minimap_Spec. _ViewExtent, _RotationMode and
    // _CategoryFilter are DISSOLVED into FFragment_Minimap_Current: all three are request-mutable,
    // so an authored copy retained here goes stale the instant a Request_Set* lands. _CategoryFilter
    // was previously mutated IN this fragment via the reflected Spec's setter, which is the only
    // reason HandleRequests took it ReadWrite - it no longer does.
    struct CKMINIMAP_API FFragment_Minimap_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Minimap_Params);

    private:
        ECk_Minimap_ProjectionMode _ProjectionMode = ECk_Minimap_ProjectionMode::ObserverCentric;
        ECk_Minimap_FrameShape _FrameShape = ECk_Minimap_FrameShape::Rectangle;
        FCk_Minimap_WorldBounds _FixedBounds;
        int32 _MaxEntries = 64;
        FCk_Time _UpdateInterval;

    public:
        CK_PROPERTY_GET(_ProjectionMode);
        CK_PROPERTY_GET(_FrameShape);
        CK_PROPERTY_GET(_FixedBounds);
        CK_PROPERTY_GET(_MaxEntries);
        CK_PROPERTY_GET(_UpdateInterval);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Minimap_Params, _ProjectionMode, _FrameShape, _FixedBounds,
            _MaxEntries, _UpdateInterval);
    };

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
        FCk_Handle _Observer;

        FCk_Handle_FogOfWar _FogOfWar;

        float _ViewExtent = 0.0f;

        ECk_Minimap_RotationMode _RotationMode = ECk_Minimap_RotationMode::NorthLocked;

        FGameplayTagQuery _CategoryFilter;

        FVector _ViewOrigin = FVector::ZeroVector;
        float _ViewYawDegrees = 0.0f;

        TArray<FCk_Minimap_Entry> _Entries;

        FCk_Time _TimeSinceUpdate;

    public:
        CK_PROPERTY_GET(_Observer);
        CK_PROPERTY_GET(_FogOfWar);
        CK_PROPERTY_GET(_ViewExtent);
        CK_PROPERTY_GET(_RotationMode);
        CK_PROPERTY_GET(_CategoryFilter);
        CK_PROPERTY_GET(_ViewOrigin);
        CK_PROPERTY_GET(_ViewYawDegrees);
        CK_PROPERTY_GET(_Entries);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Per-tick working buffers for the Update processor — retained on the entity so their
    // allocations are reused across ticks. _Entries is the BACK buffer of the published
    // Current._Entries (swapped, never copied).
    struct CKMINIMAP_API FFragment_Minimap_Scratch
    {
    public:
        CK_GENERATED_BODY(FFragment_Minimap_Scratch);

    public:
        friend class FProcessor_Minimap_Update;
        friend class FProcessor_Minimap_EndPlay;

    private:
        TArray<FCk_Minimap_Entry> _Entries;
        TArray<FCk_Entity> _PoiEntities;
        TArray<TOptional<FCk_Minimap_Entry>> _ParallelSlots;
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

    CK_DEFINE_RECORD_OF_ENTITIES_TRANSIENT(FFragment_RecordOfMinimaps, FCk_Handle_Minimap);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKMINIMAP_API, OnMinimapEntryAppeared, FCk_Delegate_Minimap_EntryAppeared, FCk_Handle_Minimap, FCk_Minimap_Entry);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKMINIMAP_API, OnMinimapEntryDisappeared, FCk_Delegate_Minimap_EntryDisappeared, FCk_Handle_Minimap, FCk_Handle_Poi);

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_Minimap_Requests);
}

// --------------------------------------------------------------------------------------------------------------------
