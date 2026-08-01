#pragma once

#include "CkWebUmg/Ir/CkWebUmg_Ir.h"

class SWidget;

// --------------------------------------------------------------------------------------------------------------------

namespace ck::webumg
{
    struct CKWEBUMG_API FCkWebUmg_BuildResult
    {
        TSharedPtr<SWidget> RootWidget;

        /**
         * IR node id -> the widget whose arranged geometry corresponds to that node's border box.
         * The fidelity harness walks this to compare arranged rects against the IR.
         */
        TMap<FString, TSharedPtr<SWidget>> WidgetsByIrId;
    };

    /** Build a live Slate widget tree from a loaded IR document (layout + solid paint only — Gate 2). */
    CKWEBUMG_API auto
    BuildWidgetTree(
        const TSharedPtr<const FCkWebUmg_IrNode>& InRoot)
        -> FCkWebUmg_BuildResult;
}

// --------------------------------------------------------------------------------------------------------------------
