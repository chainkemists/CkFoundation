#include "CkInputBias_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkInput/CkInput_Log.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_GROUP(ck::FGroup_Input_Bias);

CK_REGISTER_PROCESSOR(ck::FProcessor_InputBias_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_InputBias_CancelPendingRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_InputBias_Condition);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_input_bias_processor
{
    auto
        Get_IndexOfAxisBias(
            const TArray<FCk_InputBias_AxisBias>& InAxisBiases,
            const FKey& InAxisKey)
        -> int32
    {
        return InAxisBiases.IndexOfByPredicate(
        [&](const FCk_InputBias_AxisBias& InExisting) -> bool
        {
            return InExisting.Get_AxisKey() == InAxisKey;
        });
    }

    auto
        Get_ConditionedValue(
            float InRawValue,
            const FCk_InputBias_AxisBias& InAxisBias)
        -> float
    {
        const auto Inverted = InAxisBias.Get_Inversion() == ECk_InputBias_AxisInversion::Inverted
            ? -InRawValue
            : InRawValue;

        const auto Sign = Inverted < 0.0f ? -1.0f : 1.0f;
        const auto Magnitude = FMath::Abs(Inverted);

        const auto Deadzone = InAxisBias.Get_Deadzone();

        if (Magnitude <= Deadzone)
        { return 0.0f; }

        // The divisor is only guaranteed non-zero because both entry boundaries reject a deadzone of 1 or more.
        const auto Rescaled = (Magnitude - Deadzone) / (1.0f - Deadzone);
        const auto Curved = FMath::Pow(Rescaled, InAxisBias.Get_Exponent());

        return Sign * Curved * InAxisBias.Get_Sensitivity();
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_InputBias_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InInputBias,
            FFragment_InputBias_Params& InParams,
            FFragment_InputBias_Requests& InRequests) const
        -> void
    {
        const auto RequestsCopy = InRequests._Requests;
        InRequests._Requests.Reset();

        algo::ForEachRequest(RequestsCopy, ck::Visitor(
        [&](const auto& InRequest) -> void
        {
            auto Result = ECk_Request_OperationResult::Failed;
            const auto Guard = MakeCompletionGuard(InRequest, InInputBias, Result);

            Result = DoHandleRequest(InInputBias, InParams, InRequest);
        }), policy::DontResetContainer{});

        if (InRequests._Requests.IsEmpty())
        {
            InInputBias.Remove<MarkedDirtyBy>();
        }
    }

    auto
        FProcessor_InputBias_HandleRequests::
        DoHandleRequest(
            HandleType InInputBias,
            FFragment_InputBias_Params& InParams,
            const FCk_Request_InputBias_SetAxisBias& InRequest)
        -> ECk_Request_OperationResult
    {
        const auto& AxisBias = InRequest.Get_AxisBias();

        const auto ExistingIndex = ck_input_bias_processor::Get_IndexOfAxisBias(
            InParams._AxisBiases, AxisBias.Get_AxisKey());

        if (ExistingIndex != INDEX_NONE)
        {
            InParams._AxisBiases[ExistingIndex] = AxisBias;
        }
        else
        {
            InParams._AxisBiases.Emplace(AxisBias);
        }

        input::VeryVerbose
        (
            TEXT("InputBias [{}] now conditions axis [{}] with deadzone [{}], exponent [{}], sensitivity [{}] "
                 "and inversion [{}]"),
            InInputBias,
            AxisBias.Get_AxisKey(),
            AxisBias.Get_Deadzone(),
            AxisBias.Get_Exponent(),
            AxisBias.Get_Sensitivity(),
            AxisBias.Get_Inversion()
        );

        return ECk_Request_OperationResult::Succeeded;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_InputBias_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InInputBias,
            const FFragment_InputBias_Requests& InRequests)
        -> void
    {
        request::FireCancelledForPending(InInputBias, InRequests.Get_Requests());
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_InputBias_Condition::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InInputBias,
            const FFragment_InputBias_Params& InParams,
            FFragment_InputBias_Current& InCurrent,
            const FFragment_InputSource_Current& InSourceCurrent) const
        -> void
    {
        const auto& PendingRawEvents = InSourceCurrent.Get_PendingRawEvents();

        for (const auto& Event : PendingRawEvents)
        {
            if (Event.Get_EventType() != ECk_InputSource_EventType::AnalogAxis)
            { continue; }

            DoConditionAxis(InInputBias, InParams, InCurrent, Event);
        }
    }

    auto
        FProcessor_InputBias_Condition::
        DoConditionAxis(
            HandleType InInputBias,
            const FFragment_InputBias_Params& InParams,
            FFragment_InputBias_Current& InCurrent,
            const FCk_InputSource_RawEvent& InEvent)
        -> void
    {
        const auto& AxisKey = InEvent.Get_Key();
        const auto RawValue = InEvent.Get_AnalogValue();

        const auto BiasIndex = ck_input_bias_processor::Get_IndexOfAxisBias(InParams.Get_AxisBiases(), AxisKey);

        const auto ConditionedValue = BiasIndex != INDEX_NONE
            ? ck_input_bias_processor::Get_ConditionedValue(RawValue, InParams.Get_AxisBiases()[BiasIndex])
            : RawValue;

        auto ConditionedAxis = FCk_InputBias_ConditionedAxis{AxisKey};
        ConditionedAxis.Set_RawValue(RawValue);
        ConditionedAxis.Set_ConditionedValue(ConditionedValue);

        const auto ExistingIndex = InCurrent._ConditionedAxes.IndexOfByPredicate(
        [&](const FCk_InputBias_ConditionedAxis& InExisting) -> bool
        {
            return InExisting.Get_AxisKey() == AxisKey;
        });

        if (ExistingIndex != INDEX_NONE)
        {
            InCurrent._ConditionedAxes[ExistingIndex] = ConditionedAxis;
        }
        else
        {
            InCurrent._ConditionedAxes.Emplace(ConditionedAxis);
        }

        input::VeryVerbose
        (
            TEXT("InputBias [{}] conditioned axis [{}] from raw [{}] to [{}]"),
            InInputBias, AxisKey, RawValue, ConditionedValue
        );
    }
}

// --------------------------------------------------------------------------------------------------------------------
