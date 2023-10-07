#include "CkFloatAttribute_Fragment.h"

#include "CkAttribute/FloatAttribute/CkFloatAttribute_Utils.h"

// --------------------------------------------------------------------------------------------------------------------
auto
    UCk_Fragment_FloatAttribute_Rep::
    Broadcast_AddModifier_Implementation(
        const FGameplayTag                                    InModifierName,
        const FCk_Fragment_FloatAttributeModifier_ParamsData& InParams)
    -> void
{
    if (NOT Get_AssociatedEntity().IsValid()) { return; }
    CK_ENSURE_VALID_UNREAL_WORLD_IF_NOT(this) { return; }
    [&]()
    {
        Broadcast_AddModifier_Clients(InModifierName,
                                      InParams);
    }();
}

auto
    UCk_Fragment_FloatAttribute_Rep::
    Broadcast_AddModifier_Clients_Implementation(
        FGameplayTag                                   InModifierName,
        FCk_Fragment_FloatAttributeModifier_ParamsData InParams)
    -> void
{
    if (NOT Get_AssociatedEntity().IsValid()) { return; }
    CK_ENSURE_VALID_UNREAL_WORLD_IF_NOT(this) { return; }
    if (GetWorld()->IsNetMode(NM_DedicatedServer)) { return; }
    [&]()
    {
        UCk_Utils_FloatAttributeModifier_UE::Add(Get_AssociatedEntity(),
                                                 InModifierName,
                                                 InParams);
    }();
}

auto
    UCk_Fragment_FloatAttribute_Rep::
    Broadcast_RemoveModifier_Implementation(
        FGameplayTag InModifierName,
        FGameplayTag InAttributeName)
    -> void
{
    CK_REP_OBJ_EXECUTE_IF_VALID([&]()
    {
        Broadcast_RemoveModifier_Clients(InModifierName, InAttributeName);
    });
}

auto
    UCk_Fragment_FloatAttribute_Rep::
    Broadcast_RemoveModifier_Clients_Implementation(
        FGameplayTag InModifierName,
        FGameplayTag InAttributeName)
    -> void
{
    CK_REP_OBJ_EXECUTE_IF_VALID([&]()
    {
        UCk_Utils_FloatAttributeModifier_UE::Remove(Get_AssociatedEntity(), InAttributeName, InModifierName);
    });
}

// --------------------------------------------------------------------------------------------------------------------
