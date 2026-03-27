#pragma once

#include "CkItem_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_InventoryItem_Definition;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct CKINVENTORY_API FFragment_InventoryItem
    {
        CK_GENERATED_BODY(FFragment_InventoryItem);

    private:
        TWeakObjectPtr<const UCk_InventoryItem_Definition> _Definition;

    public:
        CK_PROPERTY_GET(_Definition);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_InventoryItem, _Definition);
    };
}

// --------------------------------------------------------------------------------------------------------------------
