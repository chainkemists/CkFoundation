#include "CkIntentSampler_Processor.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkInput/CkInputBias_Utils.h"
#include "CkInput/CkInputButtonMap_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_GROUP(ck::FGroup_Intent_Collect);
CK_REGISTER_GROUP(ck::FGroup_Intent_Sample);

CK_REGISTER_PROCESSOR(ck::FProcessor_IntentSampler_Collect);
CK_REGISTER_PROCESSOR(ck::FProcessor_IntentSampler_Sample);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_intent_sampler_processor
{
    // A key can produce SEVERAL buttons — two mappings in different categories legitimately share one — so every
    // holder is appended rather than an arbitrary first. A source with no button map appends nothing at all.
    auto
        AppendButtonsForKey(
            const FCk_Handle_InputButtonMap& InButtonMap,
            const FKey& InKey,
            TArray<FCk_Input_ButtonId>& OutButtons)
        -> void
    {
        if (ck::Is_NOT_Valid(InButtonMap))
        { return; }

        for (const auto& ButtonId : UCk_Utils_InputButtonMap_UE::Get_ButtonIdsForKey(InButtonMap, InKey))
        {
            OutButtons.AddUnique(ButtonId);
        }
    }

    // The enum's order IS the angle order, so the octant an angle falls in is one rounded division; the +1 is
    // the Neutral slot the enum reserves at zero. Rounding rather than flooring is what centres each octant on
    // its named direction instead of starting it there.
    auto
        Get_OctantAtAngle(
            float InAngleDegrees)
        -> ECk_Intent_Octant
    {
        const auto Steps = FMath::RoundToInt(InAngleDegrees / ck::intent_sampler::OctantWidthDegrees);
        const auto Index = ((Steps % ck::intent_sampler::OctantCount) + ck::intent_sampler::OctantCount)
                           % ck::intent_sampler::OctantCount;

        return static_cast<ECk_Intent_Octant>(Index + 1);
    }

    auto
        Get_OctantCentreDegrees(
            ECk_Intent_Octant InOctant)
        -> float
    {
        return (static_cast<int32>(InOctant) - 1) * ck::intent_sampler::OctantWidthDegrees;
    }

    auto
        Get_IsInQuad(
            const FCk_Intent_SocdQuad& InQuad,
            const FCk_Input_ButtonId& InButton)
        -> bool
    {
        return InButton == InQuad.Get_Up()   || InButton == InQuad.Get_Down() ||
               InButton == InQuad.Get_Left() || InButton == InQuad.Get_Right();
    }

    auto
        Get_CleanedAxis(
            const TArray<FCk_Input_ButtonId>& InHeld,
            const TArray<FCk_Input_ButtonId>& InPressOrder,
            const FCk_Input_ButtonId& InPositive,
            const FCk_Input_ButtonId& InNegative,
            ECk_Intent_SocdPolicy InPolicy)
        -> ECk_Intent_CleanedAxis
    {
        const auto PositiveIsHeld = InHeld.Contains(InPositive);
        const auto NegativeIsHeld = InHeld.Contains(InNegative);

        if (NOT PositiveIsHeld && NOT NegativeIsHeld)
        { return ECk_Intent_CleanedAxis::Neutral; }

        if (NOT NegativeIsHeld)
        { return ECk_Intent_CleanedAxis::Positive; }

        if (NOT PositiveIsHeld)
        { return ECk_Intent_CleanedAxis::Negative; }

        switch (InPolicy)
        {
            case ECk_Intent_SocdPolicy::Neutral:
            { return ECk_Intent_CleanedAxis::Neutral; }
            case ECk_Intent_SocdPolicy::LastInputPriority:
            case ECk_Intent_SocdPolicy::FirstInputPriority:
            {
                const auto PositiveOrder = InPressOrder.IndexOfByKey(InPositive);
                const auto NegativeOrder = InPressOrder.IndexOfByKey(InNegative);

                // A held cardinal normally carries a press-order entry, so a missing one means its press
                // predates the sampler. Neither end is then first or last, and Neutral is the honest answer
                // rather than a winner picked by whichever lookup failed.
                if (PositiveOrder == INDEX_NONE || NegativeOrder == INDEX_NONE)
                { return ECk_Intent_CleanedAxis::Neutral; }

                const auto PositiveWins = InPolicy == ECk_Intent_SocdPolicy::LastInputPriority
                    ? PositiveOrder > NegativeOrder
                    : PositiveOrder < NegativeOrder;

                return PositiveWins ? ECk_Intent_CleanedAxis::Positive : ECk_Intent_CleanedAxis::Negative;
            }
            default:
            {
                CK_INVALID_ENUM(InPolicy);
                return ECk_Intent_CleanedAxis::Neutral;
            }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_IntentSampler_Collect::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InSampler,
            const FFragment_IntentSampler_Params& InParams,
            FFragment_IntentSampler_PendingEvents& InPending,
            const FFragment_InputLayer_RoutedThisFrame& InRouted)
        -> void
    {
        InPending._Pending.Append(InRouted.Get_RoutedEvents());
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IntentSampler_Sample::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InSampler,
            const FFragment_IntentSampler_Params& InParams,
            FFragment_IntentSampler_Current& InCurrent,
            FFragment_IntentSampler_PendingEvents& InPending) const
        -> void
    {
        // Claimed by value and the buffer emptied before anything is derived from it: the row must own its rows
        // (it outlives the buffer), and clearing here is what leaves the rest of a catch-up burst nothing to
        // record a second time.
        const auto Claimed = InPending._Pending;
        InPending._Pending.Reset();

        auto Row = FCk_Intent_FrameRecord{InCurrent._NextFrameIndex};
        ++InCurrent._NextFrameIndex;

        Row.Set_RoutedEvents(Claimed);

        DoRecordButtons(InSampler, InCurrent, Claimed, Row);
        DoRecordAxes(InSampler, InParams, InCurrent, Claimed, Row);

        // Both derivations read what the two passes above just wrote into the row — the octant from its axis
        // pair, SOCD from its held set — so they run after them and never re-read the fragments themselves.
        DoRecordOctant(InParams, InCurrent, Row);
        DoRecordSocd(InSampler, InParams, InCurrent, Claimed, Row);

        DoAppendRow(InParams, InCurrent, Row);
    }

    auto
        FProcessor_IntentSampler_Sample::
        DoRecordButtons(
            HandleType InSampler,
            FFragment_IntentSampler_Current& InCurrent,
            const TArray<FCk_InputLayer_RoutedEvent>& InClaimed,
            FCk_Intent_FrameRecord& OutRow)
        -> void
    {
        const auto ButtonMap = UCk_Utils_InputButtonMap_UE::Cast(InSampler);

        auto Pressed  = TArray<FCk_Input_ButtonId>{};
        auto Released = TArray<FCk_Input_ButtonId>{};

        // Every claimed row updates held-ness regardless of its delivery outcome: a release a layer never
        // received is still a key that came up, and a record that ignored it would report the key held forever.
        for (const auto& Routed : InClaimed)
        {
            const auto& Event = Routed.Get_Event();
            const auto& Key   = Event.Get_Key();

            switch (Event.Get_EventType())
            {
                case ECk_InputSource_EventType::Pressed:
                {
                    InCurrent._HeldKeys.AddUnique(Key);
                    ck_intent_sampler_processor::AppendButtonsForKey(ButtonMap, Key, Pressed);
                    break;
                }
                case ECk_InputSource_EventType::Released:
                {
                    InCurrent._HeldKeys.Remove(Key);
                    ck_intent_sampler_processor::AppendButtonsForKey(ButtonMap, Key, Released);
                    break;
                }
                case ECk_InputSource_EventType::AnalogAxis:
                { break; }
                default:
                {
                    CK_INVALID_ENUM(Event.Get_EventType());
                    break;
                }
            }
        }

        auto Held = TArray<FCk_Input_ButtonId>{};

        for (const auto& HeldKey : InCurrent._HeldKeys)
        {
            ck_intent_sampler_processor::AppendButtonsForKey(ButtonMap, HeldKey, Held);
        }

        OutRow.Set_Pressed(Pressed);
        OutRow.Set_Released(Released);
        OutRow.Set_Held(Held);
    }

    auto
        FProcessor_IntentSampler_Sample::
        DoRecordAxes(
            HandleType InSampler,
            const FFragment_IntentSampler_Params& InParams,
            FFragment_IntentSampler_Current& InCurrent,
            const TArray<FCk_InputLayer_RoutedEvent>& InClaimed,
            FCk_Intent_FrameRecord& OutRow)
        -> void
    {
        const auto Bias = UCk_Utils_InputBias_UE::Cast(InSampler);

        if (ck::IsValid(Bias))
        {
            InCurrent._LastAxisX = UCk_Utils_InputBias_UE::Get_ConditionedAxisValue(Bias, InParams.Get_AxisKeyX());
            InCurrent._LastAxisY = UCk_Utils_InputBias_UE::Get_ConditionedAxisValue(Bias, InParams.Get_AxisKeyY());
        }
        else
        {
            // No conditioning stage composed is an identity passthrough, and the raw value only still exists in
            // the claimed rows — the inbox it arrived in was drained a group earlier. Reading the buffer rather
            // than one render frame's retention is what keeps the fallback from missing samples above 60 fps.
            // Values persist across frames because an axis at rest sends nothing.
            for (const auto& Routed : InClaimed)
            {
                const auto& Event = Routed.Get_Event();

                if (Event.Get_EventType() != ECk_InputSource_EventType::AnalogAxis)
                { continue; }

                if (Event.Get_Key() == InParams.Get_AxisKeyX())
                { InCurrent._LastAxisX = Event.Get_AnalogValue(); }
                else if (Event.Get_Key() == InParams.Get_AxisKeyY())
                { InCurrent._LastAxisY = Event.Get_AnalogValue(); }
            }
        }

        OutRow.Set_ConditionedAxisX(InCurrent._LastAxisX);
        OutRow.Set_ConditionedAxisY(InCurrent._LastAxisY);
    }

    auto
        FProcessor_IntentSampler_Sample::
        DoRecordOctant(
            const FFragment_IntentSampler_Params& InParams,
            FFragment_IntentSampler_Current& InCurrent,
            FCk_Intent_FrameRecord& OutRow)
        -> void
    {
        const auto AxisX = OutRow.Get_ConditionedAxisX();
        const auto AxisY = OutRow.Get_ConditionedAxisY();

        const auto Magnitude = FMath::Sqrt(FMath::Square(AxisX) + FMath::Square(AxisY));

        if (Magnitude <= InParams.Get_OctantNeutralRadius())
        {
            // The remembered octant is dropped rather than kept across the neutral pass. Hysteresis exists to
            // stop a boundary flicker inside one gesture, and a stick that came back to centre has ended the
            // gesture — carrying the memory through would let it bias the direction of the NEXT one.
            InCurrent._LastOctant = ECk_Intent_Octant::Neutral;
            OutRow.Set_Octant(ECk_Intent_Octant::Neutral);
            return;
        }

        const auto AngleDegrees = FMath::RadiansToDegrees(FMath::Atan2(AxisY, AxisX));
        const auto Candidate = ck_intent_sampler_processor::Get_OctantAtAngle(AngleDegrees);

        const auto Previous = InCurrent._LastOctant;

        if (Previous != ECk_Intent_Octant::Neutral)
        {
            const auto PreviousCentreDegrees = ck_intent_sampler_processor::Get_OctantCentreDegrees(Previous);
            const auto DistanceFromPrevious = FMath::Abs(
                FMath::FindDeltaAngleDegrees(PreviousCentreDegrees, AngleDegrees));

            // Staying costs nothing, leaving costs the margin: the previous octant holds its own half-width
            // plus the margin past each boundary, and only an angle outside THAT band adopts the candidate.
            // Asymmetry is the whole mechanism — a symmetric rule is what flickers on a boundary.
            const auto HoldBandDegrees = ck::intent_sampler::HalfOctantDegrees +
                                         InParams.Get_OctantHysteresisMarginDegrees();

            if (DistanceFromPrevious <= HoldBandDegrees)
            {
                OutRow.Set_Octant(Previous);
                return;
            }
        }

        InCurrent._LastOctant = Candidate;
        OutRow.Set_Octant(Candidate);
    }

    auto
        FProcessor_IntentSampler_Sample::
        DoRecordSocd(
            HandleType InSampler,
            const FFragment_IntentSampler_Params& InParams,
            FFragment_IntentSampler_Current& InCurrent,
            const TArray<FCk_InputLayer_RoutedEvent>& InClaimed,
            FCk_Intent_FrameRecord& OutRow)
        -> void
    {
        const auto& Quad = InParams.Get_SocdQuad();

        if (NOT Quad.Get_IsFullyNamed())
        {
            InCurrent._SocdPressOrder.Reset();
            return;
        }

        const auto ButtonMap = UCk_Utils_InputButtonMap_UE::Cast(InSampler);

        // Press order is rebuilt from the claimed events in ARRIVAL order rather than from the row's Pressed
        // set: the priority policies ask which of two buttons went down first, and a set cannot answer that
        // for two presses the same logic frame claimed.
        for (const auto& Routed : InClaimed)
        {
            const auto& Event = Routed.Get_Event();

            if (Event.Get_EventType() != ECk_InputSource_EventType::Pressed)
            { continue; }

            auto PressedButtons = TArray<FCk_Input_ButtonId>{};
            ck_intent_sampler_processor::AppendButtonsForKey(ButtonMap, Event.Get_Key(), PressedButtons);

            for (const auto& Button : PressedButtons)
            {
                if (NOT ck_intent_sampler_processor::Get_IsInQuad(Quad, Button))
                { continue; }

                // A re-press moves the entry to the end: pressing a button while its opposite is already down
                // IS the latest input, and leaving it at its original position would let a stale press outrank
                // the one the player just made.
                InCurrent._SocdPressOrder.Remove(Button);
                InCurrent._SocdPressOrder.Emplace(Button);
            }
        }

        const auto& Held = OutRow.Get_Held();

        InCurrent._SocdPressOrder.RemoveAll([&Held](const FCk_Input_ButtonId& InButton)
        {
            return NOT Held.Contains(InButton);
        });

        OutRow.Set_CleanedHorizontal(ck_intent_sampler_processor::Get_CleanedAxis(
            Held, InCurrent._SocdPressOrder, Quad.Get_Right(), Quad.Get_Left(), InParams.Get_SocdPolicy()));

        OutRow.Set_CleanedVertical(ck_intent_sampler_processor::Get_CleanedAxis(
            Held, InCurrent._SocdPressOrder, Quad.Get_Up(), Quad.Get_Down(), InParams.Get_SocdPolicy()));
    }

    auto
        FProcessor_IntentSampler_Sample::
        DoAppendRow(
            const FFragment_IntentSampler_Params& InParams,
            FFragment_IntentSampler_Current& InCurrent,
            const FCk_Intent_FrameRecord& InRow)
        -> void
    {
        const auto Capacity = InParams.Get_RingCapacity();

        // The write index and the array length advance together while the ring fills, so one branch grows the
        // storage and the other overwrites — never both.
        if (InCurrent._Rows.Num() < Capacity)
        {
            InCurrent._Rows.Emplace(InRow);
        }
        else
        {
            InCurrent._Rows[InCurrent._NextWriteIndex] = InRow;
        }

        InCurrent._NextWriteIndex = (InCurrent._NextWriteIndex + 1) % Capacity;
        InCurrent._RowCount = FMath::Min(InCurrent._RowCount + 1, Capacity);
    }
}

// --------------------------------------------------------------------------------------------------------------------
