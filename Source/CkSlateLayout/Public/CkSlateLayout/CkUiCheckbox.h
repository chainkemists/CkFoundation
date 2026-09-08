#pragma once

#include "CkSlateLayout/CkUiWidgetRegistry.h"

/** Registers the retained, model-bound checkbox custom widget. */
class CKSLATELAYOUT_API FCkUiCheckbox final
{
public:
    static auto Register(FCkUiWidgetRegistry& InRegistry) -> FCkUiLoadResult;
};
