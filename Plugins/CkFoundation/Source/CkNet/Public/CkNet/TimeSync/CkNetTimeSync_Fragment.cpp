#include "CkNetTimeSync_Fragment.h"

#include "CkNetTimeSync_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Fragment_TimeSync_Rep::
    Broadcast_TimeSync_Implementation(
        APlayerController* InPlayerController,
        float InRoundTripTime)
    -> void
{
    if (NOT Get_AssociatedEntity().IsValid()) { return; }

    CK_ENSURE_VALID_UNREAL_WORLD_IF_NOT(this) { return; }

    if (GetWorld()->IsNetMode(NM_Client))
    { return; }

    UCk_Utils_NetTimeSync_UE::Request_NewTimeSync
    (
        Get_AssociatedEntity(),
        FCk_Request_NetTimeSync_NewSync{}.Set_PlayerController(InPlayerController).Set_RoundTripTime(InRoundTripTime)
    );
}

auto
    UCk_Fragment_TimeSync_Rep::
    OnLink()
    -> void
{
    Super::OnLink();
    _AssociatedEntity.Add<TObjectPtr<ThisType>>() = this;
}

auto
    UCk_Fragment_TimeSync_Rep::
    DoBroadcast_TimeSync(
        float InRoundTripTime)
    -> void
{
    CK_REP_OBJ_EXECUTE_IF_VALID([&]()
    {
        Broadcast_TimeSync(GetWorld()->GetFirstPlayerController(), InRoundTripTime);
    });
}

// --------------------------------------------------------------------------------------------------------------------
