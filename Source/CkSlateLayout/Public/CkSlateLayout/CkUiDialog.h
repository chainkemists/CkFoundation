#pragma once

#include "CkSlateLayout/CkUiWidgetRegistry.h"

/** Registers the retained, in-surface dialog custom widget. */
class CKSLATELAYOUT_API FCkUiDialog final
{
public:
    static auto Register(FCkUiWidgetRegistry& InRegistry) -> FCkUiLoadResult;
};
