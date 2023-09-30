#include "CkMeterAttribute_Utils.h"

#include "CkAttribute/FloatAttribute/CkFloatAttribute_Fragment.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Utils.h"
#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"
#include "CkLabel/CkLabel_Utils.h"

#include <GameplayTagsManager.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_meter_attribute
{
    struct FMeterAttribute_Tags final : public FGameplayTagNativeAdder
    {
    public:
        virtual ~FMeterAttribute_Tags() = default;

    protected:
        auto AddTags() -> void override
        {
            auto& Manager = UGameplayTagsManager::Get();

            _MinCapacity = Manager.AddNativeGameplayTag(TEXT("Ck.Attribute.Meter.MinCapacity"));
            _MaxCapacity = Manager.AddNativeGameplayTag(TEXT("Ck.Attribute.Meter.MaxCapacity"));
            _Current     = Manager.AddNativeGameplayTag(TEXT("Ck.Attribute.Meter.Current"));
        }

    private:
        FGameplayTag _MinCapacity;
        FGameplayTag _MaxCapacity;
        FGameplayTag _Current;

        static FMeterAttribute_Tags _Tags;

    public:
        static auto Get_MinCapacity() -> FGameplayTag
        {
            return _Tags._MinCapacity;
        }

        static auto Get_MaxCapacity() -> FGameplayTag
        {
            return _Tags._MaxCapacity;
        }

        static auto Get_Current() -> FGameplayTag
        {
            return _Tags._Current;
        }
    };

    FMeterAttribute_Tags FMeterAttribute_Tags::_Tags;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_MeterAttribute_UE::
    Add(
        FCk_Handle                                    InHandle,
        const FCk_Fragment_MeterAttribute_ParamsData& InParams)
    -> void
{
    // NOTES: Meter Attribute is piggy-backing on the Float Attribute (including Replication)

    RecordOfMeterAttributes_Utils::AddIfMissing(InHandle, ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);

    const auto& AddNewMeterAttributeToEntity = [&](FCk_Handle InAttributeOwner, const FGameplayTag& InAttributeName, FCk_Meter InAttributeBaseValue)
    {
        const auto NewAttributeEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InAttributeOwner);

        ck::UCk_Utils_OwningEntity::Add(NewAttributeEntity, InHandle);

        const auto& MeterParams = InAttributeBaseValue.Get_Params();
        const auto& MeterCapacity = MeterParams.Get_Capacity();
        const auto& MeterStartingPercentage = MeterParams.Get_StartingPercentage();

        UCk_Utils_FloatAttribute_UE::AddMultiple
        (
            NewAttributeEntity,
            {
                { ck_meter_attribute::FMeterAttribute_Tags::Get_MinCapacity(), MeterCapacity.Get_MinCapacity() },
                { ck_meter_attribute::FMeterAttribute_Tags::Get_MaxCapacity(), MeterCapacity.Get_MaxCapacity() },
                { ck_meter_attribute::FMeterAttribute_Tags::Get_Current(), MeterStartingPercentage.Get_Value() }
            }
        );

        UCk_Utils_GameplayLabel_UE::Add(NewAttributeEntity, InAttributeName);

        RecordOfMeterAttributes_Utils::Request_Connect(InAttributeOwner, NewAttributeEntity);
    };

    AddNewMeterAttributeToEntity(InHandle, InParams.Get_AttributeName(), InParams.Get_AttributeBaseValue());
}

auto
    UCk_Utils_MeterAttribute_UE::
    AddMultiple(
        FCk_Handle                                            InHandle,
        const TArray<FCk_Fragment_MeterAttribute_ParamsData>& InParams)
    -> void
{
    for (const auto& param : InParams)
    {
        Add(InHandle, param);
    }
}

auto
    UCk_Utils_MeterAttribute_UE::
    Has(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName)
    -> bool
{
    const auto FoundEntity = RecordOfMeterAttributes_Utils::Get_RecordEntryIf(InAttributeOwnerEntity,
    [&](FCk_Handle InHandle)
    {
        return UCk_Utils_GameplayLabel_UE::Get_Label(InHandle) == InAttributeName;
    });

    if (NOT ck::IsValid(FoundEntity))
    { return false; }

    const auto& FloatAttributeEntiyy = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(InAttributeOwnerEntity, ck_meter_attribute::FMeterAttribute_Tags::Get_MinCapacity());
    return ck::IsValid(FloatAttributeEntiyy);
}

auto
    UCk_Utils_MeterAttribute_UE::
    Has_Any(
        FCk_Handle InAttributeOwnerEntity)
    -> bool
{
    return RecordOfMeterAttributes_Utils::Has(InAttributeOwnerEntity);
}

auto
    UCk_Utils_MeterAttribute_UE::
    Ensure(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InAttributeOwnerEntity, InAttributeName), TEXT("Handle [{}] does NOT have a Meter Attribute [{}]"), InAttributeOwnerEntity, InAttributeName)
    { return false; }

    return true;
}

auto
    UCk_Utils_MeterAttribute_UE::
    Ensure_Any(
        FCk_Handle InAttributeOwnerEntity)
    -> bool
{
    CK_ENSURE_IF_NOT(Has_Any(InAttributeOwnerEntity), TEXT("Handle [{}] does NOT have any Meter Attribute [{}]"), InAttributeOwnerEntity)
    { return false; }

    return true;
}

auto
    UCk_Utils_MeterAttribute_UE::
    Get_All(
        FCk_Handle InAttributeOwnerEntity)
    -> TArray<FGameplayTag>
{
    if (NOT RecordOfMeterAttributes_Utils::Has(InAttributeOwnerEntity))
    { return {}; }

    const auto& meterAttributeEntities = RecordOfMeterAttributes_Utils::Get_AllRecordEntries(InAttributeOwnerEntity);

    return ck::algo::Transform<TArray<FGameplayTag>>(meterAttributeEntities, [&](FCk_Handle InMeterAttributeEntity)
    {
        return UCk_Utils_GameplayLabel_UE::Get_Label(InMeterAttributeEntity);
    });
}

auto
    UCk_Utils_MeterAttribute_UE::
    Get_BaseValue(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName)
    -> FCk_Meter
{
    const auto [MinCapacity, MaxCapacity, Current] = Get_MinMaxAndCurrentAttributeEntities(InAttributeOwnerEntity, InAttributeName);

    const auto& MinCapacityValue =  FloatAttribute_Utils::Get_BaseValue(MinCapacity);
    const auto& MaxCapacityValue = FloatAttribute_Utils::Get_BaseValue(MaxCapacity);
    const auto& CurrentValue = FloatAttribute_Utils::Get_BaseValue(Current);

    return FCk_Meter
    {
        FCk_Meter_Params{FCk_Meter_Capacity{MaxCapacityValue}.Set_MinCapacity(MinCapacityValue)}
        .Set_StartingPercentage(FCk_FloatRange_0to1{CurrentValue})
    };
}

auto
    UCk_Utils_MeterAttribute_UE::
    Get_BonusValue(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName)
    -> FCk_Meter
{
    return Get_FinalValue(InAttributeOwnerEntity, InAttributeName) - Get_BaseValue(InAttributeOwnerEntity, InAttributeName);
}

auto
    UCk_Utils_MeterAttribute_UE::
    Get_FinalValue(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName)
    -> FCk_Meter
{
    const auto [MinCapacity, MaxCapacity, Current] = Get_MinMaxAndCurrentAttributeEntities(InAttributeOwnerEntity, InAttributeName);

    const auto& MinCapacityValue =  FloatAttribute_Utils::Get_FinalValue(MinCapacity);
    const auto& MaxCapacityValue = FloatAttribute_Utils::Get_FinalValue(MaxCapacity);
    const auto& CurrentValue = FloatAttribute_Utils::Get_FinalValue(Current);

    return FCk_Meter
    {
        FCk_Meter_Params{FCk_Meter_Capacity{MaxCapacityValue}.Set_MinCapacity(MinCapacityValue)}
        .Set_StartingPercentage(FCk_FloatRange_0to1{CurrentValue})
    };
}

auto
    UCk_Utils_MeterAttribute_UE::
    BindTo_OnValueChanged(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        ECk_Signal_BindingPolicy InBehavior,
        const FCk_Delegate_MeterAttribute_OnValueChanged& InDelegate)
    -> void
{
    const auto FoundEntity = RecordOfMeterAttributes_Utils::Get_RecordEntryIf(InAttributeOwnerEntity,
    [&](FCk_Handle InHandle)
    {
        return UCk_Utils_GameplayLabel_UE::Get_Label(InHandle) == InAttributeName;
    });

    const auto [MinCapacity, MaxCapacity, Current] = Get_MinMaxAndCurrentAttributeEntities(InAttributeOwnerEntity, InAttributeName);

    switch(InBehavior)
    {
    case ECk_Signal_BindingPolicy::FireIfPayloadInFlight:
        ck::UUtils_Signal_OnFloatAttributeValueChanged::Bind
            <&UCk_Utils_MeterAttribute_UE::OnMinCapacityChanged, ECk_Signal_BindingPolicy::FireIfPayloadInFlight>(MinCapacity);
        ck::UUtils_Signal_OnFloatAttributeValueChanged::Bind
            <&UCk_Utils_MeterAttribute_UE::OnMaxCapacityChanged, ECk_Signal_BindingPolicy::FireIfPayloadInFlight>(MaxCapacity);
        ck::UUtils_Signal_OnFloatAttributeValueChanged::Bind
            <&UCk_Utils_MeterAttribute_UE::OnCurrentValueCanged, ECk_Signal_BindingPolicy::FireIfPayloadInFlight>(Current);
        break;
    case ECk_Signal_BindingPolicy::IgnorePayloadInFlight:
        ck::UUtils_Signal_OnFloatAttributeValueChanged::Bind
            <&UCk_Utils_MeterAttribute_UE::OnMinCapacityChanged, ECk_Signal_BindingPolicy::IgnorePayloadInFlight>(MinCapacity);
        ck::UUtils_Signal_OnFloatAttributeValueChanged::Bind
            <&UCk_Utils_MeterAttribute_UE::OnMaxCapacityChanged, ECk_Signal_BindingPolicy::IgnorePayloadInFlight>(MaxCapacity);
        ck::UUtils_Signal_OnFloatAttributeValueChanged::Bind
            <&UCk_Utils_MeterAttribute_UE::OnCurrentValueCanged, ECk_Signal_BindingPolicy::IgnorePayloadInFlight>(Current);
        break;
    default:
        CK_INVALID_ENUM(InBehavior);
    }

    ck::UUtils_Signal_OnMeterAttributeValueChanged::Bind(FoundEntity, InDelegate, InBehavior);
}

auto
    UCk_Utils_MeterAttribute_UE::
    UnbindFrom_OnValueChanged(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        const FCk_Delegate_MeterAttribute_OnValueChanged& InDelegate)
    -> void
{
    const auto FoundEntity = RecordOfMeterAttributes_Utils::Get_RecordEntryIf(InAttributeOwnerEntity,
    [&](FCk_Handle InHandle)
    {
        return UCk_Utils_GameplayLabel_UE::Get_Label(InHandle) == InAttributeName;
    });

    const auto [MinCapacity, MaxCapacity, Current] = Get_MinMaxAndCurrentAttributeEntities(FoundEntity, InAttributeName);

    ck::UUtils_Signal_OnFloatAttributeValueChanged::Unbind<OnMinCapacityChanged>(MinCapacity);
    ck::UUtils_Signal_OnFloatAttributeValueChanged::Unbind<OnMaxCapacityChanged>(MaxCapacity);
    ck::UUtils_Signal_OnFloatAttributeValueChanged::Unbind<OnCurrentValueCanged>(Current);

    ck::UUtils_Signal_OnMeterAttributeValueChanged::Unbind(FoundEntity, InDelegate);
}

auto
    UCk_Utils_MeterAttribute_UE::
    Get_MinMaxAndCurrentAttributeEntities(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName)
    -> std::tuple<FCk_Handle, FCk_Handle, FCk_Handle>
{
    const auto FoundEntity = RecordOfMeterAttributes_Utils::Get_RecordEntryIf(InAttributeOwnerEntity,
    [&](FCk_Handle InHandle)
    {
        return UCk_Utils_GameplayLabel_UE::Get_Label(InHandle) == InAttributeName;
    });

    const auto& MinCapacity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(FoundEntity, ck_meter_attribute::FMeterAttribute_Tags::Get_MinCapacity());

    const auto& MaxCapacity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(FoundEntity, ck_meter_attribute::FMeterAttribute_Tags::Get_MaxCapacity());

    const auto& Current = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(FoundEntity, ck_meter_attribute::FMeterAttribute_Tags::Get_Current());

    return {MinCapacity, MaxCapacity, Current};
}

auto
    UCk_Utils_MeterAttribute_UE::
    OnMinCapacityChanged(
        FCk_Handle InFloatAttributeEntity,
        ck::TPayload_Attribute_OnValueChanged<ck::FFragment_FloatAttribute> InValueChanged)
    -> void
{
    const auto MeterAttributeEntity = InFloatAttributeEntity;
    const auto MeterAttributeLabel = UCk_Utils_GameplayLabel_UE::Get_Label(MeterAttributeEntity);
    const auto MeterAttributeOwnerEntity = ck::UCk_Utils_OwningEntity::Get_StoredEntity(MeterAttributeEntity);

    ck::UUtils_Signal_OnMeterAttributeValueChanged::Broadcast
    (
        MeterAttributeEntity,
        ck::MakePayload
        (
            MeterAttributeOwnerEntity,
            ck::TPayload_Attribute_OnValueChanged<ck::FFragment_MeterAttribute>
            {
                MeterAttributeOwnerEntity,
                Get_BaseValue(MeterAttributeOwnerEntity, MeterAttributeLabel),
                Get_FinalValue(MeterAttributeOwnerEntity, MeterAttributeLabel)
            }
        )
    );
}

auto
    UCk_Utils_MeterAttribute_UE::
    OnMaxCapacityChanged(
        FCk_Handle InFloatAttributeEntity,
        ck::TPayload_Attribute_OnValueChanged<ck::FFragment_FloatAttribute> InValueChanged)
    -> void
{
    const auto MeterAttributeEntity = InFloatAttributeEntity;
    const auto MeterAttributeLabel = UCk_Utils_GameplayLabel_UE::Get_Label(MeterAttributeEntity);
    const auto MeterAttributeOwnerEntity = ck::UCk_Utils_OwningEntity::Get_StoredEntity(MeterAttributeEntity);

    ck::UUtils_Signal_OnMeterAttributeValueChanged::Broadcast
    (
        MeterAttributeEntity,
        ck::MakePayload
        (
            MeterAttributeOwnerEntity,
            ck::TPayload_Attribute_OnValueChanged<ck::FFragment_MeterAttribute>
            {
                MeterAttributeOwnerEntity,
                Get_BaseValue(MeterAttributeOwnerEntity, MeterAttributeLabel),
                Get_FinalValue(MeterAttributeOwnerEntity, MeterAttributeLabel)
            }
        )
    );
}

auto
    UCk_Utils_MeterAttribute_UE::
    OnCurrentValueCanged(
        FCk_Handle InFloatAttributeEntity,
        ck::TPayload_Attribute_OnValueChanged<ck::FFragment_FloatAttribute> InValueChanged)
    -> void
{
    const auto MeterAttributeEntity = InFloatAttributeEntity;
    const auto MeterAttributeLabel = UCk_Utils_GameplayLabel_UE::Get_Label(MeterAttributeEntity);
    const auto MeterAttributeOwnerEntity = ck::UCk_Utils_OwningEntity::Get_StoredEntity(MeterAttributeEntity);

    ck::UUtils_Signal_OnMeterAttributeValueChanged::Broadcast
    (
        MeterAttributeEntity,
        ck::MakePayload
        (
            MeterAttributeOwnerEntity,
            ck::TPayload_Attribute_OnValueChanged<ck::FFragment_MeterAttribute>
            {
                MeterAttributeOwnerEntity,
                Get_BaseValue(MeterAttributeOwnerEntity, MeterAttributeLabel),
                Get_FinalValue(MeterAttributeOwnerEntity, MeterAttributeLabel)
            }
        )
    );
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_MeterAttributeModifier_UE::
    Add(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InModifierName,
        const FCk_Fragment_MeterAttributeModifier_ParamsData& InParams)
    -> void
{
    const auto& AttributeName = InParams.Get_TargetAttributeName();

    const auto FoundEntity = UCk_Utils_MeterAttribute_UE::RecordOfMeterAttributes_Utils::Get_RecordEntryIf(InAttributeOwnerEntity,
    [&](FCk_Handle InHandle)
    {
        return UCk_Utils_GameplayLabel_UE::Get_Label(InHandle) == AttributeName;
    });

    UCk_Utils_FloatAttributeModifier_UE::Add(FoundEntity, InModifierName,
        FCk_Fragment_FloatAttributeModifier_ParamsData{FCk_Fragment_FloatAttributeModifier_ParamsData
        {
            InParams.Get_ModifierDelta().Get_Params().Get_Capacity().Get_MinCapacity(),
            ck_meter_attribute::FMeterAttribute_Tags::Get_MinCapacity(),
            InParams.Get_ModifierOperation()
        }});

    UCk_Utils_FloatAttributeModifier_UE::Add(FoundEntity, InModifierName,
        FCk_Fragment_FloatAttributeModifier_ParamsData{FCk_Fragment_FloatAttributeModifier_ParamsData
        {
            InParams.Get_ModifierDelta().Get_Params().Get_Capacity().Get_MaxCapacity(),
            ck_meter_attribute::FMeterAttribute_Tags::Get_MaxCapacity(),
            InParams.Get_ModifierOperation()
        }});

    UCk_Utils_FloatAttributeModifier_UE::Add(FoundEntity, InModifierName,
        FCk_Fragment_FloatAttributeModifier_ParamsData{FCk_Fragment_FloatAttributeModifier_ParamsData
        {
            InParams.Get_ModifierDelta().Get_Params().Get_StartingPercentage().Get_Value(),
            ck_meter_attribute::FMeterAttribute_Tags::Get_Current(),
            InParams.Get_ModifierOperation()
        }});
}

auto
    UCk_Utils_MeterAttributeModifier_UE::
    Has(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        FGameplayTag InModifierName)
    -> bool
{
    const auto& AttributeName = InAttributeName;

    const auto FoundEntity = UCk_Utils_MeterAttribute_UE::RecordOfMeterAttributes_Utils::Get_RecordEntryIf(InAttributeOwnerEntity,
    [&](FCk_Handle InHandle)
    {
        return UCk_Utils_GameplayLabel_UE::Get_Label(InHandle) == AttributeName;
    });

    return UCk_Utils_FloatAttributeModifier_UE::Has(FoundEntity, ck_meter_attribute::FMeterAttribute_Tags::Get_MinCapacity(), InModifierName);
}

auto
    UCk_Utils_MeterAttributeModifier_UE::
    Ensure(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        FGameplayTag InModifierName)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InAttributeOwnerEntity, InAttributeName, InModifierName), TEXT("Handle [{}] does NOT have a Meter Attribute Modifier with name [{}]"), InAttributeOwnerEntity, InModifierName)
    { return false; }

    return true;
}

auto
    UCk_Utils_MeterAttributeModifier_UE::
    Remove(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        FGameplayTag InModifierName)
    -> void
{
    const auto FoundEntity = UCk_Utils_MeterAttribute_UE::RecordOfMeterAttributes_Utils::Get_RecordEntryIf(InAttributeOwnerEntity,
    [&](FCk_Handle InHandle)
    {
        return UCk_Utils_GameplayLabel_UE::Get_Label(InHandle) == InAttributeName;
    });

    UCk_Utils_FloatAttributeModifier_UE::Remove(FoundEntity, ck_meter_attribute::FMeterAttribute_Tags::Get_MinCapacity(), InModifierName);
    UCk_Utils_FloatAttributeModifier_UE::Remove(FoundEntity, ck_meter_attribute::FMeterAttribute_Tags::Get_MaxCapacity(), InModifierName);
    UCk_Utils_FloatAttributeModifier_UE::Remove(FoundEntity, ck_meter_attribute::FMeterAttribute_Tags::Get_Current(), InModifierName);
}

// --------------------------------------------------------------------------------------------------------------------
