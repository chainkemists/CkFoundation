#pragma once

#include "CkSlateLayout/CkUiWidgetRegistry.h"

/** Registers the retained, model-bound normalized Slate slider. */
class CKSLATELAYOUT_API FCkUiSlider final
{
public:
    static auto Register(FCkUiWidgetRegistry& InRegistry) -> FCkUiLoadResult;
};
