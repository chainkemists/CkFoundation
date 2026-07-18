#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------
// On-save sidecar refresh — committed sidecar exports stay truthful MECHANICALLY, not by convention: every editor
// save of a logic-bearing asset (the dispatch's exportable classes) re-exports its sibling .json/.txt in place.
// Deterministic, write-if-changed output means an unchanged asset's save produces zero diff noise.
//
// Deliberately inert for: commandlets (cooks save thousands of packages), procedural saves, and any save whose file
// target is not the package's canonical Content location (autosaves land in Saved/Autosaves — exporting those would
// stamp sidecars with UNSAVED content states). Toggle: ck.AssetExporter.AutoSidecarOnSave (default on).
// --------------------------------------------------------------------------------------------------------------------

class CKASSETEXPORTER_API FCk_AssetExporter_OnSaveHook
{
public:
    static auto
    Register() -> void;

    static auto
    Unregister() -> void;
};

// --------------------------------------------------------------------------------------------------------------------
