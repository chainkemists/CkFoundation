#include "CkInventoryItem_Definition.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkDynamic/CkDynamic_Utils.h"

#include "CkInventory/Item/CkInventoryItem_Fragment.h"
#include "CkInventory/Item/CkInventoryItem_ItemFragment.h"
#include "CkInventory/Item/CkInventoryItem_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryItem_Definition::
    DoConstruct_Implementation(
        FCk_Handle& InHandle) const
    -> void
{
    InHandle.Add<ck::FFragment_InventoryItem_Params>(
        FCk_Fragment_InventoryItem_ParamsData(
            TSoftObjectPtr<UCk_InventoryItem_Definition>(const_cast<UCk_InventoryItem_Definition*>(this))));

    auto ItemHandle = UCk_Utils_InventoryItem_UE::CastChecked(InHandle);

    for (const auto& ItemFragment : _ItemFragments)
    {
        CK_ENSURE_IF_NOT(ItemFragment.IsValid(), TEXT("DoConstruct: Invalid ItemFragment on Definition [%s]"), *GetName())
        { continue; }

        ItemFragment.Get().OnApplied(ItemHandle);

        FInstancedStruct FragmentCopy;
        FragmentCopy.InitializeAs(ItemFragment.GetScriptStruct(), ItemFragment.GetMemory());
        UCk_Utils_DynamicFragment_UE::Add_Fragment(InHandle, FragmentCopy);
    }
}

// --------------------------------------------------------------------------------------------------------------------
