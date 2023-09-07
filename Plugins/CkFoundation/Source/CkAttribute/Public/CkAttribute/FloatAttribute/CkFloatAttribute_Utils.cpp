#include "CkFloatAttribute_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"
#include "CkLabel/CkLabel_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

#define CK_EXECUTE_FUNC_ON_FLOAT_ATTRIBUTE_WITH_NAME(_AttributeName_, _Entity_, _Func_)\
    const auto& Pred = [_AttributeName_](FCk_Handle InEntity)\
    {\
        return UTT_Utils_FloatAttribute_UE::FloatAttribute_Utils::Has(InEntity) && UCk_Utils_GameplayLabel_UE::Has(InEntity) && UCk_Utils_GameplayLabel_UE::Get_Label(InEntity) == _AttributeName_;\
    };\
\
    const auto attributeEntity = Pred(_Entity_) ? _Entity_ : UTT_Utils_FloatAttribute_UE::RecordOfFloatAttributes_Utils::Get_RecordEntryIf(_Entity_, Pred);\
\
    return _Func_(attributeEntity)

// --------------------------------------------------------------------------------------------------------------------

#define CK_EXECUTE_VOID_FUNC_ON_FLOAT_ATTRIBUTE_WITH_NAME(_AttributeName_, _Entity_, _Func_)\
    const auto& Pred = [_AttributeName_](FCk_Handle InEntity)\
    {\
        return UTT_Utils_FloatAttribute_UE::FloatAttribute_Utils::Has(InEntity) && UCk_Utils_GameplayLabel_UE::Has(InEntity) && UCk_Utils_GameplayLabel_UE::Get_Label(InEntity) == _AttributeName_;\
    };\
\
    const auto attributeEntity = Pred(_Entity_) ? _Entity_ : UTT_Utils_FloatAttribute_UE::RecordOfFloatAttributes_Utils::Get_RecordEntryIf(_Entity_, Pred);\
\
     _Func_(attributeEntity)

// --------------------------------------------------------------------------------------------------------------------

auto
    UTT_Utils_FloatAttribute_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Provider_FloatAttributes_ParamsData& InParams)
    -> void
{
    // TODO: Select Record policy that disallow duplicate based on Gameplay Label
    RecordOfFloatAttributes_Utils::AddIfMissing(InHandle);

    const auto& paramsProvider = InParams.Get_Provider();

    CK_ENSURE_IF_NOT(ck::IsValid(paramsProvider), TEXT("Invalid Float Attributes Provider"))
    { return; }

    const auto& AddNewFloatAttributeToEntity = [](FCk_Handle InAttributeOwner, const FGameplayTag& InAttributeName, float InAttributeBaseValue)
    {
        const auto newAttributeEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InAttributeOwner);

        FloatAttribute_Utils::Add(newAttributeEntity, InAttributeBaseValue);
        UCk_Utils_GameplayLabel_UE::Add(newAttributeEntity, InAttributeName);

        RecordOfFloatAttributes_Utils::Request_Connect(InAttributeOwner, newAttributeEntity);
    };

    const auto& floatAttributesParams = paramsProvider->Get_Value();

    for (auto kvp : floatAttributesParams.Get_AttributeBaseValues())
    {
        const auto& attributeName = kvp.Key;
        const auto& attributeBaseValue = kvp.Value;

        AddNewFloatAttributeToEntity(InHandle, attributeName, attributeBaseValue);
    }
}

auto
    UTT_Utils_FloatAttribute_UE::
    Has(
        FGameplayTag InAttributeName,
        FCk_Handle InHandle)
    -> bool
{
    const auto& attributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(InHandle, InAttributeName);
    return ck::IsValid(attributeEntity);
}

auto
    UTT_Utils_FloatAttribute_UE::
    Ensure(
        FGameplayTag InAttributeName,
        FCk_Handle InHandle)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InAttributeName, InHandle), TEXT("Handle [{}] does NOT have a Float Attribute"), InHandle)
    { return false; }

    return true;
}

auto
    UTT_Utils_FloatAttribute_UE::
    Get_BaseValue(
        FGameplayTag InAttributeName,
        FCk_Handle InHandle)
    -> float
{
    const auto& attributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(InHandle, InAttributeName);
    return FloatAttribute_Utils::Get_BaseValue(attributeEntity);
}

auto
    UTT_Utils_FloatAttribute_UE::
    Get_BonusValue(
        FGameplayTag InAttributeName,
        FCk_Handle InHandle)
    -> float
{
    const auto& attributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(InHandle, InAttributeName);
    return FloatAttribute_Utils::Get_FinalValue(attributeEntity) - FloatAttribute_Utils::Get_BaseValue(attributeEntity);
}

auto
    UTT_Utils_FloatAttribute_UE::
    Get_FinalValue(
        FGameplayTag InAttributeName,
        FCk_Handle InHandle)
    -> float
{
    const auto& attributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(InHandle, InAttributeName);
    return FloatAttribute_Utils::Get_FinalValue(attributeEntity);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UTT_Utils_FloatAttributeModifier_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_FloatAttributeModifier_ParamsData& InParams)
    -> void
{
    const auto& attributeName = InParams.Get_TargetAttributeName();
    const auto& attributeOwner = InParams.Get_Target();

    const auto& attributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<UTT_Utils_FloatAttribute_UE::FloatAttribute_Utils, UTT_Utils_FloatAttribute_UE::RecordOfFloatAttributes_Utils>(attributeOwner, attributeName);

    FloatAttributeModifier_Utils::Add
    (
        InHandle,
        InParams.Get_ModifierDelta(),
        attributeEntity,
        InParams.Get_ModifierOperation()
    );
}

auto
    UTT_Utils_FloatAttributeModifier_UE::
    Has(
        FCk_Handle InHandle)
    -> bool
{
    return FloatAttributeModifier_Utils::Has(InHandle);
}

auto
    UTT_Utils_FloatAttributeModifier_UE::
    Ensure(
        FCk_Handle InHandle)
    -> bool
{
    return FloatAttributeModifier_Utils::Ensure(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------
