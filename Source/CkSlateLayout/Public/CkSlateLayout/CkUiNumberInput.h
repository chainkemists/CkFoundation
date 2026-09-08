#pragma once

#include "CkSlateLayout/CkUiWidgetRegistry.h"

/** Registers the retained, model-bound numeric text editor. */
class CKSLATELAYOUT_API FCkUiNumberInput final
{
public:
    static auto Register(FCkUiWidgetRegistry& InRegistry) -> FCkUiLoadResult;
};
