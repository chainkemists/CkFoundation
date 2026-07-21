#include "CkEntityPool_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_IS_VALID(FCk_Handle_PendingEntityPoolAcquire, IsValid_Policy_Default, [=](const FCk_Handle_PendingEntityPoolAcquire& InPendingAcquire)
{
    return ck::IsValid(InPendingAcquire.Get_TicketEntity());
});

// --------------------------------------------------------------------------------------------------------------------
