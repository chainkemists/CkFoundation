#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Signal/CkSignal_Utils.h"
#include "CkEcs/Signal/CkSignal_Fragment.h"

#include "CkMinimap/CkFogOfWar_Fragment_Data.h"

#include <Containers/BitArray.h>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_FogOfWar_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_FogOfWar_NeedsSetup);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_FogOfWar_Params = FCk_Fragment_FogOfWar_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    // Grid semantics: cell (X,Y) covers world [BoundsMin + Coord*CellSize, BoundsMin + (Coord+1)*CellSize) per axis;
    // index = CellY * _CellCounts.X + CellX (row-major, X fastest). The grid may overshoot the bounds' max edges by
    // up to one cell (ceil counts). An UNALLOCATED grid (_Explored empty — invalid bounds or cell budget blown at
    // Setup) degrades to fully-unfogged: every location queries as explored.
    struct CKMINIMAP_API FFragment_FogOfWar_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_FogOfWar_Current);

    public:
        friend class FProcessor_FogOfWar_Setup;
        friend class FProcessor_FogOfWar_HandleRequests;
        friend class FProcessor_FogOfWar_Update;
        friend class ::UCk_Utils_FogOfWar_UE;

    private:
        TBitArray<> _Explored;
        FIntPoint _CellCounts = FIntPoint::ZeroValue;

        // Entities whose Transform positions reveal cells every update (invalid handles are pruned)
        TArray<FCk_Handle> _Revealers;

        // Newly-set cell indices accumulated across this update's reveals — flushed as ONE batched
        // OnCellsRevealed broadcast, then reset
        TArray<int32> _NewlyRevealedScratch;

        FCk_Time _TimeSinceUpdate;

    public:
        CK_PROPERTY_GET(_Explored);
        CK_PROPERTY_GET(_CellCounts);
        CK_PROPERTY_GET(_Revealers);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKMINIMAP_API FFragment_FogOfWar_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_FogOfWar_Requests);

    public:
        friend class FProcessor_FogOfWar_HandleRequests;
        friend class ::UCk_Utils_FogOfWar_UE;

    public:
        using RequestType = std::variant<FCk_Request_FogOfWar_AddRevealer, FCk_Request_FogOfWar_RemoveRevealer,
            FCk_Request_FogOfWar_RevealLocation, FCk_Request_FogOfWar_RevealAll, FCk_Request_FogOfWar_Reset,
            FCk_Request_FogOfWar_SetExplored>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKMINIMAP_API, OnFogOfWarCellsRevealed, FCk_Delegate_FogOfWar_CellsRevealed, FCk_Handle_FogOfWar, FCk_FogOfWar_RevealedCells);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKMINIMAP_API, OnFogOfWarReset, FCk_Delegate_FogOfWar_Reset, FCk_Handle_FogOfWar);

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_FogOfWar_Requests);
}

// --------------------------------------------------------------------------------------------------------------------
