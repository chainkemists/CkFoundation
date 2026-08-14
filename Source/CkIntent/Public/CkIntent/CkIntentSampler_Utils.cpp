#include "CkIntentSampler_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkInput/CkInputSource_Utils.h"

#include "CkIntent/CkIntent_Log.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(
    UCk_Utils_IntentSampler_UE,
    FCk_Handle_IntentSampler,
    ck::FFragment_IntentSampler_Params,
    ck::FFragment_IntentSampler_Current,
    ck::FFragment_IntentSampler_PendingEvents);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IntentSampler_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_IntentSampler_ParamsData& InParams)
    -> FCk_Handle_IntentSampler
{
    const auto HandleIsValid = ck::IsValid(InHandle);
    CK_ENSURE_IF_NOT(HandleIsValid,
        TEXT("Add: invalid Handle [{}] — cannot compose an IntentSampler onto it"), InHandle)
    { return {}; }

    const auto HandleIsAnInputSource = UCk_Utils_InputSource_UE::Has(InHandle);
    CK_ENSURE_IF_NOT(HandleIsAnInputSource,
        TEXT("Add: Handle [{}] is not an InputSource, so an IntentSampler on it would have no routed events to "
             "record"), InHandle)
    { return {}; }

    const auto EntityHasNoSampler = NOT Has(InHandle);
    CK_ENSURE_IF_NOT(EntityHasNoSampler,
        TEXT("Add: Handle [{}] already carries an IntentSampler — one source has exactly one frame record, and a "
             "second would produce two disagreeing frame numberings for the same input"), InHandle)
    { return {}; }

    if (NOT DoGet_ParamsAreValid(InHandle, InParams))
    { return {}; }

    InHandle.Add<ck::FFragment_IntentSampler_Params>(InParams);
    InHandle.Add<ck::FFragment_IntentSampler_Current>();
    InHandle.Add<ck::FFragment_IntentSampler_PendingEvents>();

    ck::intent::Verbose
    (
        TEXT("IntentSampler composed onto InputSource [{}] with a [{}] row ring over axes [{}] and [{}]"),
        InHandle, InParams.Get_RingCapacity(), InParams.Get_AxisKeyX(), InParams.Get_AxisKeyY()
    );

    return CastChecked(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IntentSampler_UE::
    Get_FrameCount(
        const FCk_Handle_IntentSampler& InSampler)
    -> int32
{
    return InSampler.Get<ck::FFragment_IntentSampler_Current>().Get_RowCount();
}

auto
    UCk_Utils_IntentSampler_UE::
    Get_HeldKeys(
        const FCk_Handle_IntentSampler& InSampler)
    -> TArray<FKey>
{
    return InSampler.Get<ck::FFragment_IntentSampler_Current>().Get_HeldKeys();
}

auto
    UCk_Utils_IntentSampler_UE::
    Get_LatestFrame(
        const FCk_Handle_IntentSampler& InSampler)
    -> FCk_Intent_FrameRecord
{
    constexpr auto LatestOffset = 0;
    return TryGet_FrameAtOffset(InSampler, LatestOffset);
}

auto
    UCk_Utils_IntentSampler_UE::
    TryGet_FrameAtOffset(
        const FCk_Handle_IntentSampler& InSampler,
        int32 InOffset)
    -> FCk_Intent_FrameRecord
{
    const auto& Current = InSampler.Get<ck::FFragment_IntentSampler_Current>();

    if (InOffset < 0 || InOffset >= Current.Get_RowCount())
    { return {}; }

    const auto& Rows = Current.Get_Rows();

    // The modulus is the LIVE row count rather than the declared capacity: while the ring is still filling, the
    // write index and the array length advance together, so both phases wrap on the same value.
    const auto Slots = Rows.Num();
    const auto Index = ((Current.Get_NextWriteIndex() - 1 - InOffset) % Slots + Slots) % Slots;

    return Rows[Index];
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IntentSampler_UE::
    DoGet_ParamsAreValid(
        const FCk_Handle& InContext,
        const FCk_Fragment_IntentSampler_ParamsData& InParams)
    -> bool
{
    const auto CapacityIsPositive = InParams.Get_RingCapacity() > 0;
    CK_ENSURE_IF_NOT(CapacityIsPositive,
        TEXT("IntentSampler declaration on [{}] asks for a ring of [{}] rows — a record that retains nothing "
             "cannot be read back, and there is no meaning to give a non-positive capacity"),
        InContext, InParams.Get_RingCapacity())
    { return false; }

    const auto AxisKeysAreValid = InParams.Get_AxisKeyX().IsValid() && InParams.Get_AxisKeyY().IsValid();
    CK_ENSURE_IF_NOT(AxisKeysAreValid,
        TEXT("IntentSampler declaration on [{}] names an invalid axis key in the pair [{}] / [{}] — an axis "
             "nothing reports would record a permanent zero indistinguishable from a centred stick"),
        InContext, InParams.Get_AxisKeyX(), InParams.Get_AxisKeyY())
    { return false; }

    const auto AxisKeysAreDistinct = InParams.Get_AxisKeyX() != InParams.Get_AxisKeyY();
    CK_ENSURE_IF_NOT(AxisKeysAreDistinct,
        TEXT("IntentSampler declaration on [{}] names axis key [{}] for BOTH halves of the pair — the two are "
             "the independent axes a direction is derived from, so one key can only be a mistake"),
        InContext, InParams.Get_AxisKeyX())
    { return false; }

    const auto NeutralRadius = InParams.Get_OctantNeutralRadius();
    const auto NeutralRadiusIsInRange = NeutralRadius >= 0.0f && NeutralRadius < 1.0f;
    CK_ENSURE_IF_NOT(NeutralRadiusIsInRange,
        TEXT("IntentSampler declaration on [{}] asks for an octant neutral radius of [{}] — the radius is a "
             "fraction of full deflection, so a value at or above 1 would read every direction as neutral and "
             "a negative one names no band at all"),
        InContext, NeutralRadius)
    { return false; }

    const auto HysteresisMargin = InParams.Get_OctantHysteresisMarginDegrees();
    const auto HysteresisMarginIsInRange = HysteresisMargin >= 0.0f &&
                                           HysteresisMargin < ck::intent_sampler::HalfOctantDegrees;
    CK_ENSURE_IF_NOT(HysteresisMarginIsInRange,
        TEXT("IntentSampler declaration on [{}] asks for an octant hysteresis margin of [{}] degrees — it must "
             "clear zero and stay under half an octant ([{}] degrees), or an octant's hold band reaches past "
             "the centre of its neighbour and the neighbour can never be entered at all"),
        InContext, HysteresisMargin, ck::intent_sampler::HalfOctantDegrees)
    { return false; }

    const auto& SocdQuad = InParams.Get_SocdQuad();

    const auto SocdQuadIsWholeOrAbsent = SocdQuad.Get_IsFullyNamed() || SocdQuad.Get_IsUnnamed();
    CK_ENSURE_IF_NOT(SocdQuadIsWholeOrAbsent,
        TEXT("IntentSampler declaration on [{}] names only part of its SOCD cardinal quad — cleaning resolves "
             "OPPOSING PAIRS, so a partly named quad describes an axis with one end and there is no reading to "
             "give for it"),
        InContext)
    { return false; }

    const auto SocdQuadIsDistinct = SocdQuad.Get_IsUnnamed() || SocdQuad.Get_HasDistinctButtons();
    CK_ENSURE_IF_NOT(SocdQuadIsDistinct,
        TEXT("IntentSampler declaration on [{}] names one button twice in its SOCD cardinal quad — the four are "
             "the two opposing pairs a direction is cleaned from, so a repeat can only be a mistake and would "
             "read as an axis permanently opposing itself"),
        InContext)
    { return false; }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------
