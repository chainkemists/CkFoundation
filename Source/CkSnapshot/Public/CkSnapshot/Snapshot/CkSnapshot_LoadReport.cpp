#include "CkSnapshot_LoadReport.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Snapshot_LoadReport::
    Get_IsEntityAccountingClosed() const
    -> bool
{
    return _EntitiesRestored + _EntitiesSkipped + _EntitiesOrphaned == _EntitiesTotal;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Snapshot_LoadReport::
    Get_IsPayloadAccountingClosed() const
    -> bool
{
    return _PayloadsEnqueued + _PayloadsOnSkippedEntities + _PayloadsOnOrphanedEntities +
           _PayloadsOnUnresolvedOwner + _PayloadsDropped == _PayloadsTotal;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Snapshot_LoadReport::
    Get_IsAccountingClosed() const
    -> bool
{
    return Get_IsEntityAccountingClosed() && Get_IsPayloadAccountingClosed();
}

// --------------------------------------------------------------------------------------------------------------------
