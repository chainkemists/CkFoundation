#pragma once

#include "CkSlateLayout/CkUiWidgetRegistry.h"

/** Registers the retained, exact int32 text editor. */
class CKSLATELAYOUT_API FCkUiInt32Input final
{
public:
    static auto Register(FCkUiWidgetRegistry& InRegistry) -> FCkUiLoadResult;
};
