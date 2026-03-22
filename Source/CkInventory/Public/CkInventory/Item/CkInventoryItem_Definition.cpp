#include "CkInventoryItem_Definition.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkInventory/Item/CkInventoryItem_Fragment.h"
#include "CkInventory/Item/CkInventoryItem_ItemFragment.h"
#include "CkInventory/Item/CkInventoryItem_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

UCk_InventoryItem_Definition::UCk_InventoryItem_Definition()
{
    _ShowReplicationInEditor = false;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryItem_Definition::
    DoConstruct_Implementation(
        FCk_Handle& InHandle) const
    -> void
{
    InHandle.Add<ck::FFragment_InventoryItem_Params>(
        FCk_Fragment_InventoryItem_ParamsData(
            TWeakObjectPtr(const_cast<UCk_InventoryItem_Definition*>(this))));

    auto ItemHandle = UCk_Utils_InventoryItem_UE::CastChecked(InHandle);

    for (const auto& ItemFragment : _ItemFragments)
    {
        CK_ENSURE_IF_NOT(ck::IsValid(ItemFragment), TEXT("DoConstruct: Invalid ItemFragment on Definition [%s]"), *GetName())
        { continue; }

        ItemFragment.Get().OnApplied(ItemHandle);
    }

    UCk_Utils_Handle_UE::Set_DebugName(InHandle, *ck::Format_UE(TEXT("Item ({})"), this->GetFName()));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryItem_Definition::
    Get_ItemFragment(
        const UScriptStruct* InFragmentType) const
    -> TInstancedStruct<FCk_ItemFragment>
{
    const auto Result = ck::algo::FindIf(_ItemFragments,
    [InFragmentType](const TInstancedStruct<FCk_ItemFragment>& InFragment)
    {
        return InFragment.GetScriptStruct() == InFragmentType;
    });

    if (ck::Is_NOT_Valid(Result))
    { return {}; }

    return Result.GetValue();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryItem_Definition::
    Has_ItemFragment(
        const UScriptStruct* InFragmentType) const
    -> bool
{
    return ck::IsValid(Get_ItemFragment(InFragmentType));
}

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR

auto
    UCk_InventoryItem_Definition::
    PostEditChangeProperty(
        FPropertyChangedEvent& PropertyChangedEvent)
    -> void
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (PropertyChangedEvent.GetMemberPropertyName() != GET_MEMBER_NAME_CHECKED(UCk_InventoryItem_Definition, _ItemFragments))
    { return; }

    auto SeenTypes = TSet<const UScriptStruct*>{};

    for (auto Index = 0; Index < _ItemFragments.Num(); )
    {
        const auto& Fragment = _ItemFragments[Index];

        if (ck::Is_NOT_Valid(Fragment))
        {
            ++Index;
            continue;
        }

        if (const auto* StructType = Fragment.GetScriptStruct();
            SeenTypes.Contains(StructType))
        {
            _ItemFragments.RemoveAt(Index);
        }
        else
        {
            SeenTypes.Add(StructType);
            ++Index;
        }
    }
}

#endif

// --------------------------------------------------------------------------------------------------------------------
