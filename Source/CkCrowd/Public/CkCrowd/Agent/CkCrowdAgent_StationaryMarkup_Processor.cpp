#include "CkCrowdAgent_StationaryMarkup_Processor.h"

#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkNavigation/NavAreaMarkup/CkNavAreaMarkup_Utils.h"

#include "CkCrowd/CkCrowd_Stats.h"
#include "CkCrowd/Agent/CkCrowdAgent_NavArea.h"
#include "CkCrowd/Agent/CkCrowdAgent_StationaryMarkup_Algorithm.h"
#include "CkCrowd/Settings/CkCrowd_ProjectSettings.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_StationaryMarkup);
CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_NavMarkup_EndPlay);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::StationaryMarkup"), STAT_CkCrowd_StationaryMarkupProc, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_crowd_agent_stationary_markup
{
    // Re-painting per push-apart nudge would churn nav tiles for nothing.
    constexpr auto REPAINT_DRIFT_FRACTION = 0.5f;

    // Coarse position sampling rejects single-frame push-apart command spikes while still
    // classifying sustained physical movement promptly.
    constexpr auto STILLNESS_SAMPLE_INTERVAL_SEC = 0.25f;

}

namespace ck
{
    auto
        FProcessor_CrowdAgent_StationaryMarkup::
        Remove_Markup(
            FFragment_CrowdAgent_NavMarkup& InMarkup)
        -> void
    {
        if (InMarkup._Markup.IsValid())
        {
            UCk_Utils_NavAreaMarkup_UE::Request_Destroy(InMarkup._Markup.Get());
            InMarkup._Markup.Reset();
        }
        InMarkup._StationarySeconds = 0.0f;
        InMarkup._SecondsSincePaint = 0.0f;
        InMarkup._ConfirmationSerial = 0;
        InMarkup._ConfirmedOnMesh = false;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_StationaryMarkup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_NavMarkup& InMarkup)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_StationaryMarkupProc);

        using namespace ck_crowd_agent_stationary_markup;

        if (UCk_Utils_Crowd_Settings_UE::Get_StationaryMarkupMode() == ECk_CrowdStationaryMarkupMode::Disabled ||
            NOT UCk_Utils_Net_UE::Get_HasAuthority(InHandle))
        {
            Remove_Markup(InMarkup);
            return;
        }

        const auto Location = InTransform.Get_Transform().GetLocation();
        const auto& Settings = *UCk_Utils_Crowd_Settings_UE::Get();
        const auto StationarySpeedThreshold = Settings.Get_StationaryMarkupSpeedThreshold();
        const auto SpeedThresholdIsValid =
            ck_crowd_agent_stationary_markup_algorithm::IsValidSpeedThreshold(
                StationarySpeedThreshold);
        CK_ENSURE_IF_NOT(
            SpeedThresholdIsValid,
            TEXT("Invalid stationary-markup speed threshold [{}]"),
            StationarySpeedThreshold)
        {}
        if (NOT SpeedThresholdIsValid)
        {
            Remove_Markup(InMarkup);
            return;
        }

        // Stationary means PHYSICALLY stationary, not the Idle tag: a blocked walker plugs a
        // corridor exactly like an idle agent, and mutual pressers never go Idle at all.
        InMarkup._StillnessSampleAccumSec += static_cast<float>(InDeltaT.Get_Seconds());
        if (InMarkup._StillnessSampleAccumSec >= STILLNESS_SAMPLE_INTERVAL_SEC)
        {
            const auto SampleSeconds = InMarkup._StillnessSampleAccumSec;
            const auto SampleStart = InMarkup._StillnessSampleLoc;
            InMarkup._StillnessSampleLoc = Location;
            InMarkup._StillnessSampleAccumSec = 0.0f;

            if (NOT ck_crowd_agent_stationary_markup_algorithm::IsStationarySample(
                    SampleStart,
                    Location,
                    SampleSeconds,
                    StationarySpeedThreshold))
            {
                Remove_Markup(InMarkup);
                return;
            }
        }

        InMarkup._StationarySeconds += static_cast<float>(InDeltaT.Get_Seconds());
        if (InMarkup._StationarySeconds < Settings.Get_StationaryMarkupDelaySeconds())
        { return; }

        if (InMarkup._Markup.IsValid())
        {
            InMarkup._SecondsSincePaint += static_cast<float>(InDeltaT.Get_Seconds());

            const auto Drift = FVector::Dist2D(Location, InMarkup._MarkupLocation);
            if (Drift <= InParams.Get_Radius() * REPAINT_DRIFT_FRACTION)
            { return; }

            Remove_Markup(InMarkup);
            InMarkup._StationarySeconds = Settings.Get_StationaryMarkupDelaySeconds();
        }

        const auto HalfExtentXY = InParams.Get_Radius() * Settings.Get_StationaryMarkupExtentMultiplier();
        // Full height as the VERTICAL half-extent is deliberate: the modifier only marks polys
        // INSIDE the box, and a half-height band bottoms out above the floor and paints nothing.
        const auto HalfExtents = FVector{HalfExtentXY, HalfExtentXY, InParams.Get_Height()};

        auto GenericHandle = static_cast<FCk_Handle>(InHandle);
        InMarkup._Markup = TStrongObjectPtr{UCk_Utils_NavAreaMarkup_UE::Request_Create(
            GenericHandle,
            FTransform{FQuat::Identity, Location},
            HalfExtents,
            UCk_NavArea_CrowdAgent::StaticClass())};
        InMarkup._MarkupLocation = Location;
        InMarkup._MarkupRadiusUu = HalfExtentXY;
        InMarkup._MarkupVerticalHalfExtentUu = HalfExtents.Z;
        InMarkup._SecondsSincePaint = 0.0f;
        InMarkup._ConfirmationSerial = 0;
        InMarkup._ConfirmedOnMesh = false;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_NavMarkup_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_CrowdAgent_NavMarkup& InMarkup)
        -> void
    {
        FProcessor_CrowdAgent_StationaryMarkup::Remove_Markup(InMarkup);
    }
}

// --------------------------------------------------------------------------------------------------------------------
