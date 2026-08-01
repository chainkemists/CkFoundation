#pragma once

#include "CkWebUmg/Ir/CkWebUmg_Ir.h"

#include "Engine/Texture2D.h"
#include "Styling/SlateBrush.h"
#include "UObject/StrongObjectPtr.h"

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

        /** Brushes built per node (rounded boxes etc.) — Slate widgets hold raw brush pointers,
         *  so the result owns their lifetime; keep this result alive while the tree renders. */
        TArray<TSharedPtr<FSlateBrush>> OwnedBrushes;

        /** Textures loaded for img / background-image nodes — UE GC does not trace Slate brushes,
         *  so the result roots them (same lifetime contract as OwnedBrushes). */
        TArray<TStrongObjectPtr<UTexture2D>> OwnedTextures;
    };

    /**
     * Build a live Slate widget tree from a loaded IR document.
     * InContentBaseDir resolves asset src paths (the extracted page's directory); when empty,
     * image nodes build without textures (layout-only consumers).
     */
    CKWEBUMG_API auto
    BuildWidgetTree(
        const FCkWebUmg_IrDocument& InDocument,
        const FString& InContentBaseDir = {})
        -> FCkWebUmg_BuildResult;
}

// --------------------------------------------------------------------------------------------------------------------
