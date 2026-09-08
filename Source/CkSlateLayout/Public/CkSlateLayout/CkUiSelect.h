#pragma once

#include "CkSlateLayout/CkUiWidgetRegistry.h"

/** Registers the retained, collection-backed select control. */
class CKSLATELAYOUT_API FCkUiSelect final
{
public:
    static auto Register(FCkUiWidgetRegistry& InRegistry) -> FCkUiLoadResult;
};
