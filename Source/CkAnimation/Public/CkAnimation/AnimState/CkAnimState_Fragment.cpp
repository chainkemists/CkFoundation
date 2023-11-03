#include "CkAnimState_Fragment.h"

#include "CkAnimation/AnimState/CkAnimState_Utils.h"

#include <Net/UnrealNetwork.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FFragment_AnimState_Current::
        FFragment_AnimState_Current(
            FCk_AnimState_Current InCurrent)
        : _AnimGoal(InCurrent.Get_AnimGoal())
        , _AnimState(InCurrent.Get_AnimState())
        , _AnimCluster(InCurrent.Get_AnimCluster())
        , _AnimOverlay(InCurrent.Get_AnimOverlay())
    {
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Fragment_AnimState_Rep::
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
    -> void
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisType, _AnimGoal);
    DOREPLIFETIME(ThisType, _AnimState);
    DOREPLIFETIME(ThisType, _AnimCluster);
    DOREPLIFETIME(ThisType, _AnimOverlay);
}

auto
    UCk_Fragment_AnimState_Rep::
    OnRep_AnimGoal() -> void
{
    CK_REP_OBJ_EXECUTE_IF_VALID([&]()
    {
        UCk_Utils_AnimState_UE::Request_SetAnimGoal
        (
            Get_AssociatedEntity(),
            FCk_Request_AnimState_SetGoal{_AnimGoal}
        );
    });
}

auto
    UCk_Fragment_AnimState_Rep::
    OnRep_AnimState() -> void
{
    CK_REP_OBJ_EXECUTE_IF_VALID([&]()
    {
        UCk_Utils_AnimState_UE::Request_SetAnimState
        (
            Get_AssociatedEntity(),
            FCk_Request_AnimState_SetState{_AnimState}
        );
    });
}

auto
    UCk_Fragment_AnimState_Rep::
    OnRep_AnimCluster() -> void
{
    CK_REP_OBJ_EXECUTE_IF_VALID([&]()
    {
        UCk_Utils_AnimState_UE::Request_SetAnimCluster
        (
            Get_AssociatedEntity(),
            FCk_Request_AnimState_SetCluster{_AnimCluster}
        );
    });
}

auto
    UCk_Fragment_AnimState_Rep::
    OnRep_AnimOverlay() -> void
{
    CK_REP_OBJ_EXECUTE_IF_VALID([&]()
    {
        UCk_Utils_AnimState_UE::Request_SetAnimOverlay
        (
            Get_AssociatedEntity(),
            FCk_Request_AnimState_SetOverlay{_AnimOverlay}
        );
    });
}

// --------------------------------------------------------------------------------------------------------------------
