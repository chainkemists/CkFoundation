#include "CkIskmRenderer/Renderer/CkIskm_BatchedCrowd_Processor.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// Complete actor type: the TWeakObjectPtr member ctor static_asserts on it, and ForEachEntity calls
// AdvanceAnimation / DriveCosmetics.
#include "CkIskmRenderer/Renderer/CkIskm_BatchedCrowd_Actor.h"

namespace ck
{
    FFragment_IskmCrowd_Controller::FFragment_IskmCrowd_Controller(ACk_Iskm_BatchedCrowd_Actor* InCrowd)
        : _Crowd(InCrowd)
    {
    }

    auto
        FProcessor_IskmCrowd_Advance::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IskmCrowd_Controller& InController) const -> void
    {
        auto* Crowd = InController.Get_Crowd().Get();
        if (ck::Is_NOT_Valid(Crowd))
        { return; }

        Crowd->AdvanceAnimation(static_cast<float>(InDeltaT.Get_Seconds()));
        Crowd->DriveCosmetics();
    }
}

CK_REGISTER_PROCESSOR(ck::FProcessor_IskmCrowd_Advance);
