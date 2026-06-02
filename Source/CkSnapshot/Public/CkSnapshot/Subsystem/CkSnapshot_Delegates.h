#pragma once

#include "CkSnapshot/Snapshot/CkSnapshot_LoadReport.h"

#include "CkSnapshot_Delegates.generated.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_OneParam(FCk_Delegate_OnSaveComplete, ECk_SnapshotResult, InResult);
DECLARE_DYNAMIC_DELEGATE_OneParam(FCk_Delegate_OnLoadComplete, FCk_Snapshot_LoadReport, InReport);

// --------------------------------------------------------------------------------------------------------------------
