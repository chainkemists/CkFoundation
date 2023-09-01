#include "CkNetTimeSync_Fragment.h"

#include "CkNetTimeSync_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Fragment_NetTimeSync_Rep::
    Broadcast_NetTimeSync_Implementation(
        APlayerController* InPlayerController,
        FCk_Time InRoundTripTime)
    -> void
{
    CK_REP_OBJ_EXECUTE_IF_VALID([&]()
    {
        UCk_Utils_NetTimeSync_UE::Request_NewTimeSync
        (
            Get_AssociatedEntity(),
            FCk_Request_NetTimeSync_NewSync{}.Set_PlayerController(InPlayerController).Set_RoundTripTime(InRoundTripTime)
        );
    });
}

auto
    UCk_Fragment_NetTimeSync_Rep::
    Broadcast_TimeSync_Clients_Implementation(
        APlayerController* InPlayerController,
        FCk_Time InRoundTripTime)
    -> void
{
    CK_REP_OBJ_EXECUTE_IF_VALID_IGNORE_SERVER([&]()
    {
        if (GetWorld()->GetFirstPlayerController() == InPlayerController)
        { return; }

        UCk_Utils_NetTimeSync_UE::Request_NewTimeSync
        (
            Get_AssociatedEntity(),
            FCk_Request_NetTimeSync_NewSync{}.Set_PlayerController(InPlayerController).Set_RoundTripTime(InRoundTripTime)
        );
    });
}

auto
    UCk_Fragment_NetTimeSync_Rep::
    OnLink()
    -> void
{
    Super::OnLink();
    _AssociatedEntity.Add<TObjectPtr<ThisType>>() = this;
}

auto
    UCk_Fragment_NetTimeSync_Rep::
    DoBroadcast_NetTimeSync(
        FCk_Time InRoundTripTime)
    -> void
{
    CK_REP_OBJ_EXECUTE_IF_VALID([&]()
    {
        Broadcast_NetTimeSync(GetWorld()->GetFirstPlayerController(), InRoundTripTime);
    });
}

// --------------------------------------------------------------------------------------------------------------------
