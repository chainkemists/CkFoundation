#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------
// A long-lived request-file server so an external driver can amortize the editor-boot cost across many export batches.
// Polls <ProjectSaved>/CkAssetExporter/Requests/*.json, runs each (op = export | list | dumpGraph | quit) through the
// dispatch layer, writes <ProjectSaved>/CkAssetExporter/Results/<basename>.json, then deletes the request. Publishes a
// server.json status file for process lifetime. Clean-exits on a 10-minute idle window, a 2-hour wall-clock cap, or a
// quit request. Always returns 0.
// --------------------------------------------------------------------------------------------------------------------

class CKASSETEXPORTER_API FCk_AssetExporter_RequestLoop
{
public:
    static auto
    Run() -> int32;
};

// --------------------------------------------------------------------------------------------------------------------
