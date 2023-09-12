#include "CkTimer_Fragment.h"

#include "CkCore/Time/CkTime_Utils.h"
#include "CkTimer/CkTimer_Utils.h"
#include "Net/UnrealNetwork.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Fragment_Timer_Rep::
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
    -> void
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisType, _Chrono);
}

auto
    UCk_Fragment_Timer_Rep::
    OnRep_Chrono()
    -> void
{
    CK_REP_OBJ_EXECUTE_IF_VALID([&]()
    {
        UCk_Utils_Timer_UE::Request_OverrideTimer(Get_AssociatedEntity(), _Chrono);

    });
}

// --------------------------------------------------------------------------------------------------------------------
