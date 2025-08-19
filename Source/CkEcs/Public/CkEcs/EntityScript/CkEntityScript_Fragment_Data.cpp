#include "CkEntityScript_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_IS_VALID(FCk_Handle_PendingEntityScript, IsValid_Policy_Default, [=](const FCk_Handle_PendingEntityScript& InHandle)
{
    return ck::IsValid(InHandle.Get_EntityUnderConstruction());
});
