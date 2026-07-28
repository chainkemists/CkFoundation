#include "CkPredictedVelocity_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkPhysics/Velocity/CkVelocity_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_PredictedVelocity_Update);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_PredictedVelocity_Update::ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_PredictedVelocity_Current& InCurrent) const -> void
    {
        // Zero-dt tick = settle pass: do no time-dependent work. Early-out BEFORE any read/write so the
        // finite-difference divide is skipped AND _PreviousDeltaTime is not poisoned to 0 (which would trip the
        // Host branch's divide next tick).
        if (InDeltaT.Get_Seconds() <= 0.0f)
        { return; }

        const auto PreviousDeltaTime = InCurrent._PreviousDeltaTime;
        const auto PreviousLocation = InCurrent._PreviousLocation;

        const auto& OwnerActor = UCk_Utils_OwningActor_UE::Get_EntityOwningActor(InHandle);
        CK_ENSURE_IF_NOT(ck::IsValid(OwnerActor),
            TEXT("Entity [{}] does NOT have a valid actor owner!"), InHandle)
        { return; }

        const auto& CurrentLocation = OwnerActor->GetActorLocation();

        // Server needs to compare to the delta time of the previous tick
        const auto& NetMode = UCk_Utils_Net_UE::Get_EntityNetMode(InHandle);
        const auto DeltaTime = NetMode == ECk_Net_NetModeType::Host ?
            PreviousDeltaTime :
            InDeltaT;

        const auto& CurrentVelocity = (CurrentLocation - PreviousLocation) / DeltaTime.Get_Seconds();

        InCurrent._PreviousDeltaTime = InDeltaT;
        InCurrent._PreviousLocation = CurrentLocation;
        InCurrent._CurrentVelocity = CurrentVelocity;

        auto VelocityHandle = UCk_Utils_Velocity_UE::Cast(InHandle);

        UCk_Utils_Velocity_UE::Request_OverrideVelocity(VelocityHandle, CurrentVelocity, {});
    }
}

// --------------------------------------------------------------------------------------------------------------------