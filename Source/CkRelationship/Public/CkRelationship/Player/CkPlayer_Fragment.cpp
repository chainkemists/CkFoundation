#include "CkPlayer_Fragment.h"

#include "CkRelationship/Player/CkPlayer_Utils.h"

#include <Net/UnrealNetwork.h>
#include <Net/Core/PushModel/PushModel.h>

// --------------------------------------------------------------------------------------------------------------------
auto
    UCk_Fragment_Player_Rep::
    Broadcast_Assign(
        ECk_Player_ID InPlayerID)
    -> void
{
    _PlayerID = InPlayerID;
    MARK_PROPERTY_DIRTY_FROM_NAME(UCk_Fragment_Player_Rep, _PlayerID, this);
}

auto
    UCk_Fragment_Player_Rep::
    PostLink()
    -> void
{
    Super::PostLink();

    if (NOT UCk_Utils_Player_UE::Get_IsAssignedTo(UCk_Utils_Player_UE::Cast(Get_AssociatedEntity()), ECk_Player_ID::Unassigned))
    { return; }

    OnRep_Updated();
}

auto
    UCk_Fragment_Player_Rep::
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
    -> void
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    constexpr auto Params = FDoRepLifetimeParams{COND_None, REPNOTIFY_Always, true};

    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _PlayerID, Params);
}

auto
    UCk_Fragment_Player_Rep::
    OnRep_Updated()
    -> void
{
    auto Entity = Get_AssociatedEntity();

    if (ck::Is_NOT_Valid(Entity))
    { return; }

    if (GetWorld()->IsNetMode(NM_DedicatedServer))
    { return; }

    if (UCk_Utils_Player_UE::Has(Entity))
    {
        auto PlayerEntity = UCk_Utils_Player_UE::Cast(Entity);

        if (UCk_Utils_Player_UE::Get_IsAssignedTo(PlayerEntity, _PlayerID))
        { return; }

        UCk_Utils_Player_UE::Unassign(PlayerEntity);
    }

    if (_PlayerID == ECk_Player_ID::Unassigned)
    { return; }

    auto PlayerEntity = UCk_Utils_Player_UE::Cast(Entity);
    UCk_Utils_Player_UE::Assign(PlayerEntity, _PlayerID);
}

// --------------------------------------------------------------------------------------------------------------------
