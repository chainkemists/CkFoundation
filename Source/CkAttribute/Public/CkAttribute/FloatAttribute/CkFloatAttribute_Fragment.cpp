#include "CkFloatAttribute_Fragment.h"

#include "CkAttribute/CkAttribute_Log.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Utils.h"

#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

// --------------------------------------------------------------------------------------------------------------------
auto
    UCk_Fragment_FloatAttribute_Rep::
    Broadcast_AddModifier(
        const FGameplayTag InModifierName,
        const FCk_Fragment_FloatAttributeModifier_ParamsData& InParams)
    -> void
{
    MARK_PROPERTY_DIRTY_FROM_NAME(UCk_Fragment_FloatAttribute_Rep, _PendingAddModifiers, this);
    _PendingAddModifiers.Emplace(FCk_Fragment_FloatAttribute_PendingModifier{InModifierName, InParams});
}

auto
    UCk_Fragment_FloatAttribute_Rep::
    Broadcast_RemoveModifier(
        FGameplayTag InModifierName,
        FGameplayTag InAttributeName,
        ECk_MinMaxCurrent InAttributeComponent)
    -> void
{
    MARK_PROPERTY_DIRTY_FROM_NAME(UCk_Fragment_FloatAttribute_Rep, _PendingRemoveModifiers, this);
    _PendingRemoveModifiers.Emplace(FCk_Fragment_FloatAttribute_RemovePendingModifier{InAttributeName, InModifierName, InAttributeComponent});
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

auto
    UCk_Fragment_FloatAttribute_Rep::
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
    -> void
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    constexpr auto Params = FDoRepLifetimeParams{COND_None, REPNOTIFY_Always, true};

    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _PendingAddModifiers, Params);
    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _PendingRemoveModifiers, Params);
}

auto
    UCk_Fragment_FloatAttribute_Rep::
    OnRep_PendingModifiers()
    -> void
{
    if (ck::Is_NOT_Valid(Get_AssociatedEntity()))
    { return; }

    if (GetWorld()->IsNetMode(NM_DedicatedServer))
    { return; }

    for (auto Index = _NextPendingAddModifier; Index < _PendingAddModifiers.Num(); ++Index)
    {
        const auto& Modifier = _PendingAddModifiers[Index];
        const auto& AttributeName = Modifier.Get_Params().Get_TargetAttributeName();

        CK_LOG_ERROR_IF_NOT(ck::attribute, ck::IsValid(AttributeName),
            TEXT("Received a FLOAT AddModifier Request from the SERVER with ModifierName [{}] for a TargetAttribute with INVALID name.{}"),
            Modifier.Get_ModifierName(), ck::Context(this))
        { continue; }

        auto Attribute = UCk_Utils_FloatAttribute_UE::TryGet(_AssociatedEntity, AttributeName);

        CK_LOG_ERROR_IF_NOT(ck::attribute, ck::IsValid(Attribute),
            TEXT("Received a FLOAT AddModifier Request from the SERVER with ModifierName [{}] for a TargetAttribute with name [{}] "
                "but could NOT find that Attribute on [{}]"),
            Modifier.Get_ModifierName(), AttributeName, Get_AssociatedEntity())
        { continue; }

        UCk_Utils_FloatAttributeModifier_UE::Add(Attribute, Modifier.Get_ModifierName(), Modifier.Get_Params());
    }
    _NextPendingAddModifier = _PendingAddModifiers.Num();

    for (auto Index = _NextPendingRemoveModifier; Index < _PendingRemoveModifiers.Num(); ++Index)
    {
        const auto& Modifier = _PendingRemoveModifiers[Index];
        const auto& AttributeName = Modifier.Get_AttributeName();

        CK_LOG_ERROR_IF_NOT(ck::attribute, ck::IsValid(AttributeName),
            TEXT("Received a FLOAT RemoveModifier from the SERVER with Modifier [{}] for a TargetAttribute with INVALID name.{}"),
            Modifier.Get_ModifierName(),
            ck::Context(this))
        { continue; }

        const auto& Attribute = UCk_Utils_FloatAttribute_UE::TryGet(_AssociatedEntity, AttributeName);

        CK_LOG_ERROR_IF_NOT(ck::attribute, ck::IsValid(Attribute),
            TEXT("Received a FLOAT RemoveModifier Request from the SERVER with ModifierName [{}] for a TargetAttribute with name [{}] "
                "but could NOT find that Attribute on [{}]"),
            Modifier.Get_ModifierName(), AttributeName, Get_AssociatedEntity())
        { continue; }

        const auto& ModifierName = Modifier.Get_ModifierName();
        const auto& Component = Modifier.Get_Component();

        auto ModifierEntity = UCk_Utils_FloatAttributeModifier_UE::TryGet(Attribute, ModifierName, Component);

        CK_LOG_ERROR_IF_NOT(ck::attribute, ck::IsValid(ModifierEntity),
            TEXT("Received a FLOAT RemoveModifier Request from the SERVER with ModifierName [{}] for a TargetAttribute [{}] but the Modifier does NOT exist.{}"),
            ModifierName, AttributeName, ck::Context(Get_AssociatedEntity()))
        { continue; }

        UCk_Utils_FloatAttributeModifier_UE::Remove(ModifierEntity);
    }
    _NextPendingRemoveModifier = _PendingRemoveModifiers.Num();
}

// --------------------------------------------------------------------------------------------------------------------
