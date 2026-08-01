#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

class CKASSETEXPORTER_API FCk_AssetExporter_AutoReimportGuard
{
public:
    // Warns when a sidecar extension this module writes under Content/ is one the editor's auto-reimport monitor
    // will pick up, naming the config line that fixes it. A consuming project that hits this silently loses most of
    // its editor frame rate with no UE-side profiler able to attribute it, so the whole point is to make the trap
    // announce itself once at boot. Editor-only; a no-op under commandlets. Never ensures: the condition is a
    // project configuration state, not a code defect.
    static auto
    Validate() -> void;

    // A ';'-terminated concatenation of every extension an editor import factory claims, in the exact shape
    // DirectoryWatcher::MatchExtensionString consumes. Public so a test can assert the sidecar extension is absent
    // from it — that absence IS the mechanism, and nothing else would catch a future extension change that
    // silently re-enters the monitor's view.
    static auto
    Get_AllFactoryExtensions() -> FString;
};

// --------------------------------------------------------------------------------------------------------------------
