#include "CkCrowdAgent_StationaryMarkup_Processor.h"

#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkNavigation/NavAreaMarkup/CkNavAreaMarkup_Utils.h"

#include "CkCrowd/CkCrowd_Stats.h"
#include "CkCrowd/Agent/CkCrowdAgent_NavArea.h"
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

    // ~84uu/s at the default radius: well below any real walking speed, well above the aggregate
    // drift a pressed queue member accumulates from push-apart shoves.
    constexpr auto STILLNESS_SAMPLE_INTERVAL_SEC = 0.25f;
    constexpr auto STILLNESS_MAX_DRIFT_RADIUS_FRACTION = 0.5f;

    // Process-wide, not per-world: only monotonicity matters, and paths and discs are only ever
    // compared within one world.
    static auto GPaintSerial = uint64{0};
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
        InMarkup._ConfirmedOnMesh = false;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_CrowdAgent_StationaryMarkup::
        Get_CurrentPaintSerial()
        -> uint64
    {
        return ck_crowd_agent_stationary_markup::GPaintSerial;
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

        // Stationary means PHYSICALLY stationary, not the Idle tag: a blocked walker plugs a
        // corridor exactly like an idle agent, and mutual pressers never go Idle at all.
        InMarkup._StillnessSampleAccumSec += static_cast<float>(InDeltaT.Get_Seconds());
        if (InMarkup._StillnessSampleAccumSec >= STILLNESS_SAMPLE_INTERVAL_SEC)
        {
            const auto MovedUu = FVector::Dist2D(Location, InMarkup._StillnessSampleLoc);
            InMarkup._StillnessSampleLoc = Location;
            InMarkup._StillnessSampleAccumSec = 0.0f;

            if (MovedUu > InParams.Get_Radius() * STILLNESS_MAX_DRIFT_RADIUS_FRACTION)
            {
                Remove_Markup(InMarkup);
                return;
            }
        }

        const auto& Settings = *UCk_Utils_Crowd_Settings_UE::Get();

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
        InMarkup._PaintSerial = ++ck_crowd_agent_stationary_markup::GPaintSerial;
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
