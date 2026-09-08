#pragma once

#include "CkSlateLayout/CkUiWidgetRegistry.h"

/** Registers the retained, model-bound text-input custom widget. */
class CKSLATELAYOUT_API FCkUiTextInput final
{
public:
    /** Creates the same detached retained editor used by the registered tag, for shared control composition. */
    static auto Create(const FCkUiCustomWidgetArguments& InArguments, FString& OutFailure) -> TSharedPtr<ICkUiRetainedWidget>;
    static auto Register(FCkUiWidgetRegistry& InRegistry) -> FCkUiLoadResult;
};
