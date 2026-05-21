#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <CoreMinimal.h>

#include "CkInventoryUI_DragDropPolicy.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * Generic drag-and-drop access policy for inventory UI widgets.
 *
 * Set on an inventory panel (which propagates it to its item slots) to gate drag-and-drop
 * independently of the inventory's data-layer accept/stack rules. Use for read-only,
 * vendor-display, loot-only or deposit-only inventory views.
 *
 * Defaults allow everything, so existing inventory views are unaffected.
 */
USTRUCT(BlueprintType)
struct CKINVENTORY_API FCk_InventoryUI_DragDropPolicy
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_InventoryUI_DragDropPolicy);

private:
    /** Whether items dragged from ANOTHER inventory may be dropped INTO this view. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    bool _AllowDropIn = true;

    /** Whether items may be dragged OUT of this view. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    bool _AllowTakeOut = true;

public:
    CK_PROPERTY(_AllowDropIn);
    CK_PROPERTY(_AllowTakeOut);
};

// --------------------------------------------------------------------------------------------------------------------
