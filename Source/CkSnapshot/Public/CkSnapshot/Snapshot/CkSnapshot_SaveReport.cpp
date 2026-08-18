#include "CkSnapshot/Snapshot/CkSnapshot_SaveReport.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Snapshot_SaveReport::
    Add_Loss(
        FCk_Snapshot_SaveLossRecord InRecord)
    -> void
{
    _Losses.Emplace(MoveTemp(InRecord));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Snapshot_SaveReport::
    Add_UncapturedRuntimeEntity(
        FCk_Snapshot_UncapturedRuntimeRecord InRecord)
    -> void
{
    _UncapturedRuntimeEntities.Emplace(MoveTemp(InRecord));
}

// --------------------------------------------------------------------------------------------------------------------
