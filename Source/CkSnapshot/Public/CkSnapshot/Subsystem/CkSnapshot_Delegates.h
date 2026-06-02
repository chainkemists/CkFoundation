#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSnapshot/Snapshot/CkSnapshot_LoadReport.h"

#include "CkSnapshot_Delegates.generated.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_OneParam(FCk_Delegate_OnSaveComplete, ECk_SnapshotResult, InResult);
DECLARE_DYNAMIC_DELEGATE_OneParam(FCk_Delegate_OnLoadComplete, FCk_Snapshot_LoadReport, InReport);

// --------------------------------------------------------------------------------------------------------------------

// UHT requires at least one top-level reflected type in a header for its .generated.h to be valid and for
// the file-scope dynamic delegates above to be registered. This empty struct is that anchor -- it carries no
// data and is never instantiated.
USTRUCT()
struct FCk_Snapshot_DelegatesAnchor
{
    GENERATED_BODY()
};

// --------------------------------------------------------------------------------------------------------------------
