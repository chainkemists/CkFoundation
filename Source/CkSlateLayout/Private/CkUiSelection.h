#pragma once

#include "Types/SlateEnums.h"

namespace ck_ui_selection
{
    template<typename TList, typename TItem>
    auto SelectOnly(TList& InList, const TItem& InItem) -> void
    {
        if (InList.GetNumItemsSelected() == 1 && InList.IsItemSelected(InItem)) { return; }
        // SetItemSelection is additive even in Single mode. Select this already-validated key exactly.
        // Both adapters ignore Direct notifications and dispatch their own one semantic change.
        InList.ClearSelection();
        InList.SetItemSelection(InItem, true, ESelectInfo::Direct);
    }
}
