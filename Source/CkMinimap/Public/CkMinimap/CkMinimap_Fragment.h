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

    using FFragment_Minimap_Params = FCk_Minimap_Spec;

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

        FVector _ViewOrigin = FVector::ZeroVector;
        float _ViewYawDegrees = 0.0f;

        TArray<FCk_Minimap_Entry> _Entries;

        TArray<FCk_Minimap_Entry> _ScratchEntries;

        TArray<FCk_Entity> _ScratchPoiEntities;
        TArray<TOptional<FCk_Minimap_Entry>> _ScratchParallelSlots;

        FCk_Time _TimeSinceUpdate;

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

    CK_DEFINE_RECORD_OF_ENTITIES_TRANSIENT(FFragment_RecordOfMinimaps, FCk_Handle_Minimap);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKMINIMAP_API, OnMinimapEntryAppeared, FCk_Delegate_Minimap_EntryAppeared, FCk_Handle_Minimap, FCk_Minimap_Entry);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKMINIMAP_API, OnMinimapEntryDisappeared, FCk_Delegate_Minimap_EntryDisappeared, FCk_Handle_Minimap, FCk_Handle_Poi);

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_Minimap_Requests);
}

// --------------------------------------------------------------------------------------------------------------------
