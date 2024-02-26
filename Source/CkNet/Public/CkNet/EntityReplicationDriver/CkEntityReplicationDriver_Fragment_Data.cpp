#include "CkEntityReplicationDriver_Fragment_Data.h"

#include "CkNet/CkNet_Log.h"
#include "CkNet/CkNet_Utils.h"
#include "CkNet/EntityReplicationDriver/CkEntityReplicationDriver_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_HandleReplicator::
    NetSerialize(
        FArchive& Ar,
        UPackageMap* Map,
        bool& bOutSuccess)
    -> bool
{
    if (Ar.IsSaving())
    {
        if (ck::IsValid(_Handle))
        {
            UCk_Utils_Ecs_Net_UE::TryUpdateReplicatedFragment<UCk_Fragment_EntityReplicationDriver_Rep>(_Handle,
            [&](UCk_Fragment_EntityReplicationDriver_Rep* InRepDriver)
            {
                Ar << InRepDriver;
            });
        }
        else
        {
            Ar << _Handle_RepObj;
        }
    }

    if (Ar.IsLoading())
    {
        Ar << _Handle_RepObj;
        if (ck::IsValid(_Handle_RepObj))
        {
            _Handle = _Handle_RepObj->Get_AssociatedEntity();
        }
        else
        {
            _Handle = {};
        }
    }

    bOutSuccess = true;
    return true;
}

// --------------------------------------------------------------------------------------------------------------------
