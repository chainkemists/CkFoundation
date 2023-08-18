#include "CkEntity_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
UCk_Utils_Entity_UE::
Break_Entity(const FCk_Entity& InEntity,
    int32& OutEntityID,
    int32& OutEntityNumber,
    int32& OutEntityVersion)
-> void
{
    OutEntityID = static_cast<int32>(InEntity.Get_ID());
    OutEntityNumber = InEntity.Get_EntityNumber();
    OutEntityVersion = InEntity.Get_VersionNumber();
}
