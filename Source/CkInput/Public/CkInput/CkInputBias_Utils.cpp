#include "CkInputBias_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkInput/CkInputSource_Utils.h"
#include "CkInput/CkInput_Log.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(
    UCk_Utils_InputBias_UE,
    FCk_Handle_InputBias,
    ck::FFragment_InputBias_Params,
    ck::FFragment_InputBias_Current);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_input_bias_utils
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
        Get_IndexOfConditionedAxis(
            const TArray<FCk_InputBias_ConditionedAxis>& InConditionedAxes,
            const FKey& InAxisKey)
        -> int32
    {
        return InConditionedAxes.IndexOfByPredicate(
        [&](const FCk_InputBias_ConditionedAxis& InExisting) -> bool
        {
            return InExisting.Get_AxisKey() == InAxisKey;
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InputBias_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_InputBias_ParamsData& InParams)
    -> FCk_Handle_InputBias
{
    const auto HandleIsValid = ck::IsValid(InHandle);
    CK_ENSURE_IF_NOT(HandleIsValid,
        TEXT("Add: invalid Handle [{}] — cannot compose an InputBias onto it"), InHandle)
    { return {}; }

    const auto HandleIsAnInputSource = UCk_Utils_InputSource_UE::Has(InHandle);
    CK_ENSURE_IF_NOT(HandleIsAnInputSource,
        TEXT("Add: Handle [{}] is not an InputSource, so an InputBias on it would have no raw events to condition "
             "and nothing would ever sample its conditioned state"), InHandle)
    { return {}; }

    const auto EntityIsNotAlreadyBiased = NOT Has(InHandle);
    CK_ENSURE_IF_NOT(EntityIsNotAlreadyBiased,
        TEXT("Add: Handle [{}] already carries an InputBias — one source has exactly one conditioning table"),
        InHandle)
    { return {}; }

    if (NOT DoGet_AxisBiasesAreValid(InHandle, InParams.Get_AxisBiases()))
    { return {}; }

    InHandle.Add<ck::FFragment_InputBias_Params>(InParams);
    InHandle.Add<ck::FFragment_InputBias_Current>();

    ck::input::Verbose
    (
        TEXT("InputBias composed onto InputSource [{}] with [{}] declared axis biases"),
        InHandle, InParams.Get_AxisBiases().Num()
    );

    return CastChecked(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InputBias_UE::
    Get_AxisBiases(
        const FCk_Handle_InputBias& InInputBias)
    -> TArray<FCk_InputBias_AxisBias>
{
    return InInputBias.Get<ck::FFragment_InputBias_Params>().Get_AxisBiases();
}

auto
    UCk_Utils_InputBias_UE::
    TryGet_AxisBias(
        const FCk_Handle_InputBias& InInputBias,
        const FKey& InAxisKey)
    -> FCk_InputBias_AxisBias
{
    const auto& AxisBiases = InInputBias.Get<ck::FFragment_InputBias_Params>().Get_AxisBiases();

    const auto FoundIndex = ck_input_bias_utils::Get_IndexOfAxisBias(AxisBiases, InAxisKey);

    if (FoundIndex == INDEX_NONE)
    { return {}; }

    return AxisBiases[FoundIndex];
}

auto
    UCk_Utils_InputBias_UE::
    Get_ConditionedAxisValue(
        const FCk_Handle_InputBias& InInputBias,
        const FKey& InAxisKey)
    -> float
{
    const auto& ConditionedAxes = InInputBias.Get<ck::FFragment_InputBias_Current>().Get_ConditionedAxes();

    const auto FoundIndex = ck_input_bias_utils::Get_IndexOfConditionedAxis(ConditionedAxes, InAxisKey);

    if (FoundIndex == INDEX_NONE)
    { return 0.0f; }

    return ConditionedAxes[FoundIndex].Get_ConditionedValue();
}

auto
    UCk_Utils_InputBias_UE::
    Get_LastRawAxisValue(
        const FCk_Handle_InputBias& InInputBias,
        const FKey& InAxisKey)
    -> float
{
    const auto& ConditionedAxes = InInputBias.Get<ck::FFragment_InputBias_Current>().Get_ConditionedAxes();

    const auto FoundIndex = ck_input_bias_utils::Get_IndexOfConditionedAxis(ConditionedAxes, InAxisKey);

    if (FoundIndex == INDEX_NONE)
    { return 0.0f; }

    return ConditionedAxes[FoundIndex].Get_RawValue();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InputBias_UE::
    Request_SetAxisBias(
        FCk_Handle_InputBias& InInputBias,
        const FCk_Request_InputBias_SetAxisBias& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_InputBias
{
    const auto BiasIsValid = ck::IsValid(InInputBias);
    CK_ENSURE_IF_NOT(BiasIsValid,
        TEXT("Request_SetAxisBias: invalid InputBias handle [{}] — the retune is dropped"), InInputBias)
    {
        InDelegate.ExecuteIfBound(InInputBias, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InInputBias;
    }

    if (NOT DoGet_AxisBiasIsValid(InInputBias, InRequest.Get_AxisBias()))
    {
        InDelegate.ExecuteIfBound(InInputBias, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InInputBias;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InInputBias.AddOrGet<ck::FFragment_InputBias_Requests>()._Requests.Emplace(InRequest);

    return InInputBias;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InputBias_UE::
    DoGet_AxisBiasIsValid(
        const FCk_Handle& InContext,
        const FCk_InputBias_AxisBias& InAxisBias)
    -> bool
{
    const auto AxisKeyIsValid = InAxisBias.Get_AxisKey().IsValid();
    CK_ENSURE_IF_NOT(AxisKeyIsValid,
        TEXT("InputBias declaration on [{}] names an invalid axis key, so it could never condition anything"),
        InContext)
    { return false; }

    const auto Deadzone = InAxisBias.Get_Deadzone();

    const auto DeadzoneIsInRange = Deadzone >= 0.0f && Deadzone < 1.0f;
    CK_ENSURE_IF_NOT(DeadzoneIsInRange,
        TEXT("InputBias declaration on [{}] gives axis [{}] a deadzone of [{}] — it must be in [0, 1), since a "
             "deadzone of 1 kills the axis outright and leaves the rescale undefined"),
        InContext, InAxisBias.Get_AxisKey(), Deadzone)
    { return false; }

    const auto Exponent = InAxisBias.Get_Exponent();

    const auto ExponentIsPositive = Exponent > 0.0f;
    CK_ENSURE_IF_NOT(ExponentIsPositive,
        TEXT("InputBias declaration on [{}] gives axis [{}] an exponent of [{}] — it must be greater than zero, "
             "since zero flattens every deflection to full and a negative one inverts the response"),
        InContext, InAxisBias.Get_AxisKey(), Exponent)
    { return false; }

    const auto Sensitivity = InAxisBias.Get_Sensitivity();

    const auto SensitivityIsPositive = Sensitivity > 0.0f;
    CK_ENSURE_IF_NOT(SensitivityIsPositive,
        TEXT("InputBias declaration on [{}] gives axis [{}] a sensitivity of [{}] — it must be greater than zero; "
             "a flipped axis is asked for with inversion, which keeps the magnitude honest"),
        InContext, InAxisBias.Get_AxisKey(), Sensitivity)
    { return false; }

    return true;
}

auto
    UCk_Utils_InputBias_UE::
    DoGet_AxisBiasesAreValid(
        const FCk_Handle& InContext,
        const TArray<FCk_InputBias_AxisBias>& InAxisBiases)
    -> bool
{
    for (auto Index = 0; Index < InAxisBiases.Num(); ++Index)
    {
        const auto& AxisBias = InAxisBiases[Index];

        if (NOT DoGet_AxisBiasIsValid(InContext, AxisBias))
        { return false; }

        const auto FirstIndexForKey = ck_input_bias_utils::Get_IndexOfAxisBias(InAxisBiases, AxisBias.Get_AxisKey());

        const auto AxisKeyIsUnclaimed = FirstIndexForKey == Index;
        CK_ENSURE_IF_NOT(AxisKeyIsUnclaimed,
            TEXT("InputBias declaration on [{}] declares axis [{}] more than once — an axis has exactly one bias, "
                 "and silently keeping one of the two would make which one arbitrary"),
            InContext, AxisBias.Get_AxisKey())
        { return false; }
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
