#pragma once

#include "CkSlateLayout/CkUiWidgetRegistry.h"

/** Registers a compact, visual-only status pill with independently bound foreground and outline colors. */
class CKSLATELAYOUT_API FCkUiStatusPill final
{
public:
    static auto Register(FCkUiWidgetRegistry& InRegistry) -> FCkUiLoadResult;
};
