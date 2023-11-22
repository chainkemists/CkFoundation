#include "CkMeterAttribute_Processor.h"

#include "GameplayTagsManager.h"

#include "CkAttribute/MeterAttribute/CkMeterAttribute_Utils.h"

// --------------------------------------------------------------------------------------------------------------------


namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Meter_MinClamp::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle) const
        -> void
    {
        using FloatAttribute_Utils = UCk_Utils_MeterAttribute_UE::FloatAttribute_Utils;
        using RecordOfFloatAttributes_Utils = UCk_Utils_MeterAttribute_UE::RecordOfFloatAttributes_Utils;

        const auto& MinCapacity = UCk_Utils_MeterAttribute_UE::Get_EntityOrRecordEntry_WithFragmentAndLabel
            <FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(InHandle, ck::FMeterAttribute_Tags::Get_MinCapacity());

        auto Current = UCk_Utils_MeterAttribute_UE::Get_EntityOrRecordEntry_WithFragmentAndLabel
            <FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(InHandle, ck::FMeterAttribute_Tags::Get_Current());

        const auto BaseValue = UCk_Utils_MeterAttribute_UE::FloatAttribute_Utils::Get_BaseValue(Current);
        const auto FinalValue = UCk_Utils_MeterAttribute_UE::FloatAttribute_Utils::Get_FinalValue(Current);

        const auto FinalValue_Min = UCk_Utils_MeterAttribute_UE::FloatAttribute_Utils::Get_FinalValue(MinCapacity);

        const auto ClampedBaseValue = FMath::Max(BaseValue, FinalValue_Min);
        const auto ClampedFinalValue = FMath::Max(FinalValue, FinalValue_Min);

        auto& FloatAttribute = Current.Get<ck::FFragment_FloatAttribute>();
        FloatAttribute = FFragment_FloatAttribute{ClampedBaseValue, ClampedFinalValue};
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Meter_MaxClamp::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle) const
        -> void
    {
        using FloatAttribute_Utils = UCk_Utils_MeterAttribute_UE::FloatAttribute_Utils;
        using RecordOfFloatAttributes_Utils = UCk_Utils_MeterAttribute_UE::RecordOfFloatAttributes_Utils;

        const auto& MaxCapacity = UCk_Utils_MeterAttribute_UE::Get_EntityOrRecordEntry_WithFragmentAndLabel
            <FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(InHandle, ck::FMeterAttribute_Tags::Get_MaxCapacity());

        auto Current = UCk_Utils_MeterAttribute_UE::Get_EntityOrRecordEntry_WithFragmentAndLabel
            <FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(InHandle, ck::FMeterAttribute_Tags::Get_Current());

        const auto BaseValue = UCk_Utils_MeterAttribute_UE::FloatAttribute_Utils::Get_BaseValue(Current);
        const auto FinalValue = UCk_Utils_MeterAttribute_UE::FloatAttribute_Utils::Get_FinalValue(Current);

        const auto FinalValue_Max = UCk_Utils_MeterAttribute_UE::FloatAttribute_Utils::Get_FinalValue(MaxCapacity);

        const auto ClampedBaseValue = FMath::Min(BaseValue, FinalValue_Max);
        const auto ClampedFinalValue = FMath::Min(FinalValue, FinalValue_Max);

        auto& FloatAttribute = Current.Get<ck::FFragment_FloatAttribute>();
        FloatAttribute = FFragment_FloatAttribute{ClampedBaseValue, ClampedFinalValue};
    }

    // --------------------------------------------------------------------------------------------------------------------

    FProcessor_Meter_Clamp::
        FProcessor_Meter_Clamp(
        const RegistryType& InRegistry)
        : _Registry(InRegistry)
        , _MinClamp(InRegistry)
        , _MaxClamp(InRegistry)
    {
    }

    auto
        FProcessor_Meter_Clamp::
        Tick(
            TimeType InDeltaT)
        -> void
    {
        _MinClamp.Tick(InDeltaT);
        _MaxClamp.Tick(InDeltaT);
        _Registry.Clear<MarkedDirtyBy>();
    }
}
