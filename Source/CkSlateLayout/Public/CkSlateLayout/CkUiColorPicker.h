#pragma once

#include "CkSlateLayout/CkUiWidgetRegistry.h"

/** Registers the retained, model-bound Slate color-picker swatch. */
class CKSLATELAYOUT_API FCkUiColorPicker final
{
public:
    static auto Register(FCkUiWidgetRegistry& InRegistry) -> FCkUiLoadResult;
};

