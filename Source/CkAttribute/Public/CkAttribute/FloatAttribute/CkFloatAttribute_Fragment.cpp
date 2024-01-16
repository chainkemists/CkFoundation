#include "CkFloatAttribute_Fragment.h"

#include "CkAttribute/FloatAttribute/CkFloatAttribute_Utils.h"
#include "CkAttribute/MeterAttribute/CkMeterAttribute_Utils.h"

#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

// --------------------------------------------------------------------------------------------------------------------
auto
    UCk_Fragment_FloatAttribute_Rep::
    Broadcast_AddModifier_Implementation(
        const FGameplayTag                                    InModifierName,
        const FCk_Fragment_FloatAttributeModifier_ParamsData& InParams)
    -> void
{
    _PendingAddModifiers.Emplace(FCk_Fragment_FloatAttribute_PendingModifier{InModifierName, InParams});
    MARK_PROPERTY_DIRTY_FROM_NAME(ThisType, _PendingAddModifiers, this);

}

void
    UCk_Fragment_FloatAttribute_Rep::
    Broadcast_RemoveModifier_Implementation(
        FGameplayTag InModifierName,
        FGameplayTag InAttributeName)
{
    _PendingRemoveModifiers.Emplace(FCk_Fragment_FloatAttribute_RemovePendingModifier{InAttributeName, InModifierName});
    MARK_PROPERTY_DIRTY_FROM_NAME(ThisType, _PendingRemoveModifiers, this);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Fragment_FloatAttribute_Rep::
    PostLink()
    -> void
{
    OnRep_PendingModifiers();
}

// --------------------------------------------------------------------------------------------------------------------

void
    UCk_Fragment_FloatAttribute_Rep::GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    FDoRepLifetimeParams Params;
    Params.bIsPushBased = true;
    Params.Condition = COND_Custom;

    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _PendingAddModifiers, Params);
    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _PendingRemoveModifiers, Params);
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

    auto AssociatedEntity = Get_AssociatedEntity();

    for (auto Index = _NextPendingAddModifier; Index < _PendingAddModifiers.Num(); ++Index)
    {
        const auto& Modifier = _PendingAddModifiers[Index];

        CK_ENSURE_IF_NOT(ck::IsValid(Modifier.Get_Params().Get_TargetAttributeName()),
            TEXT("Received a AddModifier Request from the SERVER with name [{}].{}"),
            Modifier.Get_ModifierName(),
            ck::Context(this))
        { continue; }

        UCk_Utils_FloatAttributeModifier_UE::Add(AssociatedEntity, Modifier.Get_ModifierName(), Modifier.Get_Params());
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

        UCk_Utils_FloatAttributeModifier_UE::Remove(AssociatedEntity,
            _PendingRemoveModifiers[Index].Get_AttributeName(),
            _PendingRemoveModifiers[Index].Get_ModifierName());
    }
    _NextPendingRemoveModifier = _PendingRemoveModifiers.Num();

    // HACK: this is not a great solution i.e. for the FloatAttribute to add the Tag to the Owner if the Owner has a Meter
    // TODO: We need a better solution
    AssociatedEntity.AddOrGet<ck::FTag_Meter_RequiresUpdate>();
}
