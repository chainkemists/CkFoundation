#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Signal/CkSignal_Macros.h"

#include "CkSnapshot/Snapshot/CkSnapshot_LoadReport.h"

#include "CkSnapshot_Signals.generated.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_Snapshot_OnPreSave,
    FCk_Handle, InHandle);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Snapshot_OnSaveComplete,
    FCk_Handle, InHandle,
    ECk_SnapshotResult, InResult);

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_Snapshot_OnPreLoad,
    FCk_Handle, InHandle);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Snapshot_OnLoadComplete,
    FCk_Handle, InHandle,
    FCk_Snapshot_LoadReport, InReport);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKSNAPSHOT_API,
        Snapshot_OnPreSave,
        FCk_Delegate_Snapshot_OnPreSave,
        FCk_Handle);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKSNAPSHOT_API,
        Snapshot_OnSaveComplete,
        FCk_Delegate_Snapshot_OnSaveComplete,
        FCk_Handle,
        ECk_SnapshotResult);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKSNAPSHOT_API,
        Snapshot_OnPreLoad,
        FCk_Delegate_Snapshot_OnPreLoad,
        FCk_Handle);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKSNAPSHOT_API,
        Snapshot_OnLoadComplete,
        FCk_Delegate_Snapshot_OnLoadComplete,
        FCk_Handle,
        FCk_Snapshot_LoadReport);
}

// --------------------------------------------------------------------------------------------------------------------
