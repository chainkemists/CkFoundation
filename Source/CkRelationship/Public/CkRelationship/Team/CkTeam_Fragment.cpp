#include "CkTeam_Fragment.h"

#include "CkRelationship/Team/CkTeam_Utils.h"

#include <Net/UnrealNetwork.h>
#include <Net/Core/PushModel/PushModel.h>

// --------------------------------------------------------------------------------------------------------------------
auto
    UCk_Fragment_Team_Rep::
    Broadcast_Assign(
        ECk_Team_ID InTeamID)
    -> void
{
    _TeamID = InTeamID;
    MARK_PROPERTY_DIRTY_FROM_NAME(UCk_Fragment_Team_Rep, _TeamID, this);
}

auto
    UCk_Fragment_Team_Rep::
    PostLink()
    -> void
{
    Super::PostLink();

    OnRep_Updated();
}

auto
    UCk_Fragment_Team_Rep::
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
    -> void
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    constexpr auto Params = FDoRepLifetimeParams{COND_None, REPNOTIFY_Always, true};

    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _TeamID, Params);
}

auto
    UCk_Fragment_Team_Rep::
    OnRep_Updated()
    -> void
{
    auto Entity = Get_AssociatedEntity();

    if (ck::Is_NOT_Valid(Entity))
    { return; }

    if (GetWorld()->IsNetMode(NM_DedicatedServer))
    { return; }

    if (UCk_Utils_Team_UE::Has(Entity))
    {
        auto TeamEntity = UCk_Utils_Team_UE::Cast(Entity);
        UCk_Utils_Team_UE::Unassign(TeamEntity);
    }

    if (_TeamID == ECk_Team_ID::Unassigned)
    { return; }

    UCk_Utils_Team_UE::Assign(Entity, _TeamID);
}

// --------------------------------------------------------------------------------------------------------------------
