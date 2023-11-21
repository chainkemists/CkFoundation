#pragma once

#include "CkAttribute/CkAttribute_Processor.h"

#include "CkAttribute/MeterAttribute/CkMeterAttribute_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    class CKATTRIBUTE_API FProcessor_Meter_MinClamp
        : public TProcessor<FProcessor_Meter_MinClamp, FTag_MinValue, FTag_CurrentValue>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKATTRIBUTE_API FProcessor_Meter_MaxClamp
        : public TProcessor<FProcessor_Meter_MaxClamp, FTag_MaxValue, FTag_CurrentValue>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle) const -> void;
    };
}
