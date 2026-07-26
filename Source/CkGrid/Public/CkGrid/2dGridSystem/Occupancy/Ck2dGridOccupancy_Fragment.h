#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Tag/CkTag.h"
#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

#include "CkRecord/Record/CkRecord_Fragment.h"

#include "CkGrid/2dGridSystem/Placement/Ck2dGridPlacement_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_2dGridOccupancy_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_2dGridOccupancy_StampCells;

    // --------------------------------------------------------------------------------------------------------------------

    // CkRecord auto-prunes a dead placement from this record (reverse-link) when its entity dies.
    CK_DEFINE_RECORD_OF_ENTITIES_ROUNDTRIP(FFragment_RecordOf_GridPlacements, FCk_Handle_2dGridPlacement);

    using RecordOf_GridPlacements_Utils = ck::TUtils_RecordOfEntities<ck::FFragment_RecordOf_GridPlacements>;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKGRID_API FFragment_2dGridOccupancy_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_2dGridOccupancy_Current);

    public:
        friend class FProcessor_2dGridOccupancy_StampCells;
        friend class ::UCk_Utils_2dGridOccupancy_UE;

    private:
        TMap<FIntPoint, FCk_Handle_2dGridPlacement> _StampedCells;

    public:
        CK_PROPERTY_GET(_StampedCells);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKGRID_API FFragment_2dGridOccupant_PlacementRef
    {
    public:
        CK_GENERATED_BODY(FFragment_2dGridOccupant_PlacementRef);

    public:
        friend class ::UCk_Utils_2dGridOccupancy_UE;

    private:
        FCk_Handle_2dGridPlacement _Placement;

    public:
        CK_PROPERTY_GET(_Placement);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_2dGridOccupant_PlacementRef, _Placement);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_ECS_TAG(FTag_2dGridOccupancy_MayRequireReplication);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKGRID_API FFragment_2dGridOccupancy_SyncReplication
    {
    public:
        CK_GENERATED_BODY(FFragment_2dGridOccupancy_SyncReplication);

    private:
        TArray<FCk_2dGridPlacement_ReplicatedEntry> _PlacementsToReplicate;
        TArray<FCk_2dGridPlacement_ReplicatedEntry> _PlacementsToReplicate_Previous;

    public:
        CK_PROPERTY_GET(_PlacementsToReplicate);
        CK_PROPERTY_GET(_PlacementsToReplicate_Previous);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_2dGridOccupancy_SyncReplication, _PlacementsToReplicate, _PlacementsToReplicate_Previous);
    };

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_ContainerRef_2dGridPlacements = TFragment_ContainerEntryRef<FCk_RepData_2dGridPlacements>;
}

// --------------------------------------------------------------------------------------------------------------------
