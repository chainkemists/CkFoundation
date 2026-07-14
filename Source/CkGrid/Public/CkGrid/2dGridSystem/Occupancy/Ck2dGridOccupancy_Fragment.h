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

    // Grid-side record of the placements currently registered on this grid. CkRecord
    // auto-prunes a dead placement (reverse-link) when its entity is destroyed.
    CK_DEFINE_RECORD_OF_ENTITIES_ROUNDTRIP(FFragment_RecordOf_GridPlacements, FCk_Handle_2dGridPlacement);

    using RecordOf_GridPlacements_Utils = ck::TUtils_RecordOfEntities<ck::FFragment_RecordOf_GridPlacements>;

    // --------------------------------------------------------------------------------------------------------------------

    // Authoritative "what is currently stamped on the grid". Reconcile diffs this against
    // the desired set computed from the placement record.
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

    // Back-reference added to an OCCUPANT entity so its death-watch can find and destroy
    // the placement it owns (capture-free, member-handler friendly).
    struct CKGRID_API FFragment_2dGridOccupant_PlacementRef
    {
    public:
        CK_GENERATED_BODY(FFragment_2dGridOccupant_PlacementRef);

    public:
        friend class ::UCk_Utils_2dGridOccupancy_UE;

    public:
        // Tier-C: single entity handle, remapped through FSnapshotContext (the complete context type
        // comes in via the CkRecord fragment include above). NOTE: the death-watch DELEGATE binding
        // that pairs with this back-ref is NOT restored (signal bindings never round-trip) — a
        // restored occupant's destruction will not auto-destroy its placement.
        using IsSnapshotable = void;

        auto SerializeSnapshot(FArchive& InAr, ck::FSnapshotContext& InCtx) -> void
        {
            InCtx.Snapshot_Handle(InAr, _Placement);
        }

    private:
        FCk_Handle_2dGridPlacement _Placement;

    public:
        CK_PROPERTY_GET(_Placement);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_2dGridOccupant_PlacementRef, _Placement);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Authority-side dirty marker: set whenever the grid's placement record changes. The
    // AuthorityOnly Replicate processor consumes it, rebuilds the RepData from the live record,
    // and pushes it into the container (mirrors FTag_Inventory_MayRequireReplication).
    CK_DEFINE_ECS_TAG(FTag_2dGridOccupancy_MayRequireReplication);

    // --------------------------------------------------------------------------------------------------------------------

    // Client-side apply payload. The RegisterLazy handler stamps the incoming (current) and
    // previous replicated entries here; the ClientOnly SyncReplication processor diffs them and
    // rebuilds / tears down client placement entities (mirrors FFragment_Inventory_Spatial_SyncReplication).
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
