#pragma once

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Utils.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkLabel/CkLabel_Utils.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkNet/CkNet_Utils.h"

#include "CkEcsBasics_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKECSBASICS_API UCk_Utils_Ecs_Base_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Ecs_Base_UE);

protected:
    template <typename T_FragmentUtils, typename T_RecordUtils>
    static auto Get_EntityOrRecordEntry_WithFragmentAndLabel(
        FCk_Handle InHandle,
        FGameplayTag InLabelName) -> FCk_Handle;
};

// --------------------------------------------------------------------------------------------------------------------

template <typename T_FragmentUtils, typename T_RecordUtils>
auto
    UCk_Utils_Ecs_Base_UE::
    Get_EntityOrRecordEntry_WithFragmentAndLabel(
        FCk_Handle InHandle,
        FGameplayTag InLabelName)
    -> FCk_Handle
{
    const auto& Pred = [&InLabelName](FCk_Handle InEntity)
    {
        return T_FragmentUtils::Has(InEntity) && UCk_Utils_GameplayLabel_UE::Has(InEntity) && UCk_Utils_GameplayLabel_UE::Get_Label(InEntity).MatchesTagExact(InLabelName);
    };

    if (Pred(InHandle))
    { return InHandle; }

    return T_RecordUtils::Get_RecordEntryIf(InHandle, Pred);
}

// --------------------------------------------------------------------------------------------------------------------
