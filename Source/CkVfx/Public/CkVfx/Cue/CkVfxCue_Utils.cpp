#include "CkVfxCue_Utils.h"

#include "CkVfx/CkVfx_Log.h"
#include "CkVfxCue_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VfxCue_UE::
    Add(
        FCk_Handle& InHandle,
        const UCk_VfxCue_EntityScript& InVfxCueScript)
    -> FCk_Handle_VfxCue
{
    ck::vfx::VeryVerbose(TEXT("Adding VfxCue feature to Entity [{}]"), InHandle);

    CK_ENSURE_IF_NOT(InVfxCueScript.Get_IsConfigurationValid(),
        TEXT("VfxCue configuration is invalid for Entity [{}]"), InHandle)
    { return {}; }

    InHandle.Add<ck::FFragment_VfxCue_Current>();
    InHandle.Add<ck::FTag_VfxCue_NeedsSetup>();

    UCk_Utils_Handle_UE::Set_DebugName(InHandle, *ck::Format_UE(TEXT("VfxCue: {}"), InVfxCueScript.Get_CueName()));

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_VfxCue_UE, FCk_Handle_VfxCue,
    ck::FFragment_VfxCue_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VfxCue_UE::
    Request_Play(
        FCk_Handle_VfxCue& InVfxCue,
        const FCk_Request_VfxCue_Play& InRequest)
    -> FCk_Handle_VfxCue
{
    ck::vfx::Verbose(TEXT("Requesting play for VfxCue [{}]"), InVfxCue);

    InVfxCue.AddOrGet<ck::FFragment_VfxCue_Requests>()._Requests.Emplace(InRequest);

    return InVfxCue;
}

auto
    UCk_Utils_VfxCue_UE::
    Request_Stop(
        FCk_Handle_VfxCue& InVfxCue)
    -> FCk_Handle_VfxCue
{
    ck::vfx::Verbose(TEXT("Requesting stop for VfxCue [{}]"), InVfxCue);

    InVfxCue.AddOrGet<ck::FFragment_VfxCue_Requests>()._Requests.Emplace(
        FCk_Request_VfxCue_Stop{});

    return InVfxCue;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VfxCue_UE::
    BindTo_OnStarted(
        FCk_Handle_VfxCue& InVfxCue,
        const FCk_Delegate_VfxCue_OnStarted& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_VfxCue
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnVfxCue_Started, InVfxCue, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InVfxCue;
}

auto
    UCk_Utils_VfxCue_UE::
    BindTo_OnFinished(
        FCk_Handle_VfxCue& InVfxCue,
        const FCk_Delegate_VfxCue_OnFinished& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_VfxCue
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnVfxCue_Finished, InVfxCue, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InVfxCue;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VfxCue_UE::
    UnbindFrom_OnStarted(
        FCk_Handle_VfxCue& InVfxCue,
        const FCk_Delegate_VfxCue_OnStarted& InDelegate)
    -> FCk_Handle_VfxCue
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnVfxCue_Started, InVfxCue, InDelegate);
    return InVfxCue;
}

auto
    UCk_Utils_VfxCue_UE::
    UnbindFrom_OnFinished(
        FCk_Handle_VfxCue& InVfxCue,
        const FCk_Delegate_VfxCue_OnFinished& InDelegate)
    -> FCk_Handle_VfxCue
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnVfxCue_Finished, InVfxCue, InDelegate);
    return InVfxCue;
}

// --------------------------------------------------------------------------------------------------------------------
