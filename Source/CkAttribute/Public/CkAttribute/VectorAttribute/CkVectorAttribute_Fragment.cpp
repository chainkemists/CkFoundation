#include "CkVectorAttribute_Fragment.h"

#include "CkAttribute/VectorAttribute/CkVectorAttribute_Utils.h"

#include "Net/UnrealNetwork.h"

// --------------------------------------------------------------------------------------------------------------------
auto
    UCk_Fragment_VectorAttribute_Rep::
    Broadcast_AddModifier_Implementation(
        const FGameplayTag                                    InModifierName,
        const FCk_Fragment_VectorAttributeModifier_ParamsData& InParams)
    -> void
{
    _PendingAddModifiers.Emplace(FCk_Fragment_VectorAttribute_PendingModifier{InModifierName, InParams});
}

void
    UCk_Fragment_VectorAttribute_Rep::
    Broadcast_RemoveModifier_Implementation(
        FGameplayTag InModifierName,
        FGameplayTag InAttributeName)
{
    _PendingRemoveModifiers.Emplace(FCk_Fragment_VectorAttribute_RemovePendingModifier{InAttributeName, InModifierName});
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Fragment_VectorAttribute_Rep::
    PostLink()
    -> void
{
    OnRep_PendingModifiers();
}

// --------------------------------------------------------------------------------------------------------------------

void
    UCk_Fragment_VectorAttribute_Rep::GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisType, _PendingAddModifiers);
    DOREPLIFETIME(ThisType, _PendingRemoveModifiers);
}

auto
    UCk_Fragment_VectorAttribute_Rep::
    OnRep_PendingModifiers()
    -> void
{
    if (NOT ck::IsValid(Get_AssociatedEntity()))
    { return; }

    if (GetWorld()->IsNetMode(NM_DedicatedServer))
    { return; }

    for (auto Index = _NextPendingAddModifier; Index < _PendingAddModifiers.Num(); ++Index)
    {
        const auto& Modifier = _PendingAddModifiers[Index];

        CK_ENSURE_IF_NOT(ck::IsValid(Modifier.Get_Params().Get_TargetAttributeName()),
            TEXT("Received a AddModifier Request from the SERVER with name [{}].{}"),
            Modifier.Get_ModifierName(),
            ck::Context(this))
        { continue; }

        auto AttributeOwnerEntity = UCk_Utils_VectorAttribute_UE::Conv_HandleToVectorAttributeOwner(Get_AssociatedEntity());
        UCk_Utils_VectorAttributeModifier_UE::Add(AttributeOwnerEntity, Modifier.Get_ModifierName(), Modifier.Get_Params());
    }
    _NextPendingAddModifier = _PendingAddModifiers.Num();

    for (auto Index = _NextPendingRemoveModifier; Index < _PendingRemoveModifiers.Num(); ++Index)
    {
        const auto& Modifier = _PendingRemoveModifiers[Index];

        CK_ENSURE_IF_NOT(ck::IsValid(Modifier.Get_AttributeName()),
            TEXT("Received a RemoveModifier from the SERVER with name [{}].{}"),
            Modifier.Get_ModifierName(),
            ck::Context(this))
        { continue; }

        auto AttributeOwnerEntity = UCk_Utils_VectorAttribute_UE::Conv_HandleToVectorAttributeOwner(Get_AssociatedEntity());
        UCk_Utils_VectorAttributeModifier_UE::Remove(AttributeOwnerEntity,
            _PendingRemoveModifiers[Index].Get_AttributeName(),
            _PendingRemoveModifiers[Index].Get_ModifierName());
    }
    _NextPendingRemoveModifier = _PendingRemoveModifiers.Num();
}
