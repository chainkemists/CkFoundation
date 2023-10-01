#include "CkEntityReplicationDriver_Utils.h"

#include "CkNet/EntityReplicationDriver/CkEntityReplicationDriver_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------
auto
    UCk_Utils_EntityReplicationDriver_UE::
    Add(
        const FCk_Handle InHandle)
    -> void
{
    TryAddReplicatedFragment<UCk_Fragment_EntityReplicationDriver_Rep>(InHandle);
}
