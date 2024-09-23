#include "CkAnimState_Fragment.h"

#include "CkAnimation/AnimState/CkAnimState_Utils.h"

#include "Net/Core/PushModel/PushModel.h"

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

    constexpr auto Params = FDoRepLifetimeParams{COND_None, REPNOTIFY_Always, true};

    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _AnimGoal, Params);
    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _AnimState, Params);
    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _AnimCluster, Params);
    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _AnimOverlay, Params);
}

auto
    UCk_Fragment_AnimState_Rep::
    OnRep_AnimGoal() -> void
{
    CK_REP_OBJ_EXECUTE_IF_VALID([&]()
    {
        auto AnimStateHandle = UCk_Utils_AnimState_UE::CastChecked(_AssociatedEntity);
        UCk_Utils_AnimState_UE::Request_SetAnimGoal
        (
            AnimStateHandle,
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
        auto AnimStateHandle = UCk_Utils_AnimState_UE::CastChecked(_AssociatedEntity);
        UCk_Utils_AnimState_UE::Request_SetAnimState
        (
            AnimStateHandle,
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
        auto AnimStateHandle = UCk_Utils_AnimState_UE::CastChecked(_AssociatedEntity);
        UCk_Utils_AnimState_UE::Request_SetAnimCluster
        (
            AnimStateHandle,
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
        auto AnimStateHandle = UCk_Utils_AnimState_UE::CastChecked(_AssociatedEntity);
        UCk_Utils_AnimState_UE::Request_SetAnimOverlay
        (
            AnimStateHandle,
            FCk_Request_AnimState_SetOverlay{_AnimOverlay}
        );
    });
}

auto
    UCk_Fragment_AnimState_Rep::
    Set_AnimGoal(
        const FCk_AnimState_Goal& OutAnimGoal)
    -> void
{
    _AnimGoal = OutAnimGoal;
    MARK_PROPERTY_DIRTY_FROM_NAME(ThisType, _AnimGoal, this);
}

auto
    UCk_Fragment_AnimState_Rep::
    Set_AnimState(
        const FCk_AnimState_State& OutAnimState)
    -> void
{
    _AnimState = OutAnimState;
    MARK_PROPERTY_DIRTY_FROM_NAME(ThisType, _AnimState, this);
}

auto
    UCk_Fragment_AnimState_Rep::
    Set_AnimCluster(
        const FCk_AnimState_Cluster& OutAnimCluster)
    -> void
{
    _AnimCluster = OutAnimCluster;
    MARK_PROPERTY_DIRTY_FROM_NAME(ThisType, _AnimCluster, this);
}

auto
    UCk_Fragment_AnimState_Rep::
    Set_AnimOverlay(
        const FCk_AnimState_Overlay& OutAnimOverlay)
    -> void
{
    _AnimOverlay = OutAnimOverlay;
    MARK_PROPERTY_DIRTY_FROM_NAME(ThisType, _AnimOverlay, this);
}

// --------------------------------------------------------------------------------------------------------------------
