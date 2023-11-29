#pragma once

#include "CkAttribute/CkAttribute_Processor.h"

#include "CkAttribute/MeterAttribute/CkMeterAttribute_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    class CKATTRIBUTE_API FProcessor_Meter_MinClamp
        : public TProcessor<FProcessor_Meter_MinClamp, FTag_Meter_MinValue, FTag_Meter_CurrentValue, FTag_Meter_RequiresUpdate>
    {
    public:
        using MarkedDirtyBy = FTag_Meter_RequiresUpdate;

    public:
        using TProcessor::TProcessor;

        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKATTRIBUTE_API FProcessor_Meter_MaxClamp
        : public TProcessor<FProcessor_Meter_MaxClamp, FTag_Meter_MaxValue, FTag_Meter_CurrentValue, FTag_Meter_RequiresUpdate>
    {
    public:
        using MarkedDirtyBy = FTag_Meter_RequiresUpdate;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKATTRIBUTE_API FProcessor_Meter_Clamp
    {
    public:
        using TimeType     = FCk_Time;
        using RegistryType = FCk_Registry;
        using MarkedDirtyBy = FTag_Meter_RequiresUpdate;

    public:
        explicit
        FProcessor_Meter_Clamp(
            const RegistryType& InRegistry);

    public:
        auto Tick(
            TimeType InDeltaT) -> void;

    private:
        RegistryType _Registry;

    private:
        FProcessor_Meter_MinClamp _MinClamp;
        FProcessor_Meter_MaxClamp _MaxClamp;
    };
}
