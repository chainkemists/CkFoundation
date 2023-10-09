#include "CkFloatAttribute_Fragment.h"

#include "CkAttribute/FloatAttribute/CkFloatAttribute_Utils.h"

#include "Net/UnrealNetwork.h"

// --------------------------------------------------------------------------------------------------------------------
auto
    UCk_Fragment_FloatAttribute_Rep::
    Broadcast_AddModifier_Implementation(
        const FGameplayTag                                    InModifierName,
        const FCk_Fragment_FloatAttributeModifier_ParamsData& InParams)
    -> void
{
    _PendingAddModifiers.Emplace(FCk_Fragment_FloatAttribute_PendingModifier{InModifierName, InParams});
}

void
    UCk_Fragment_FloatAttribute_Rep::
    Broadcast_RemoveModifier_Implementation(
        FGameplayTag InModifierName,
        FGameplayTag InAttributeName)
{
    _PendingRemoveModifiers.Emplace(FCk_Fragment_FloatAttribute_RemovePendingModifier{InAttributeName, InModifierName});
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Fragment_FloatAttribute_Rep::
    OnLink()
    -> void
{
    _AssociatedEntity.Add<TObjectPtr<ThisType>>() = this;
    OnRep_PendingModifiers();
}

// --------------------------------------------------------------------------------------------------------------------

void
    UCk_Fragment_FloatAttribute_Rep::GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisType, _PendingAddModifiers);
    DOREPLIFETIME(ThisType, _PendingRemoveModifiers);
}

auto
    UCk_Fragment_FloatAttribute_Rep::
    OnRep_PendingModifiers()
    -> void
{
    if (NOT Get_AssociatedEntity().IsValid())
    { return; }

    if (GetWorld()->IsNetMode(NM_DedicatedServer))
    { return; }

    for (auto Index = _NextPendingAddModifier; Index < _PendingAddModifiers.Num(); ++Index)
    {
        UCk_Utils_FloatAttributeModifier_UE::Add(Get_AssociatedEntity(),
            _PendingAddModifiers[Index].Get_ModifierName(),
            _PendingAddModifiers[Index].Get_Params());
    }
    _NextPendingAddModifier = _PendingAddModifiers.Num();

    for (auto Index = _NextPendingRemoveModifier; Index < _PendingRemoveModifiers.Num(); ++Index)
    {
        UCk_Utils_FloatAttributeModifier_UE::Remove(Get_AssociatedEntity(),
            _PendingRemoveModifiers[Index].Get_AttributeName(),
            _PendingRemoveModifiers[Index].Get_ModifierName());
    }
    _NextPendingRemoveModifier = _PendingRemoveModifiers.Num();
}
