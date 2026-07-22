#include "CkDynamic_Processor.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkDynamic/CkDynamic_Utils.h"

#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Fragment.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_DynamicFragment_Replicate);

namespace ck
{
    // ---- Replicate (Server-side) ----

    auto
        FProcessor_DynamicFragment_Replicate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_DynamicFragment_ReplicatedTypes& InReplicatedTypes) const
        -> void
    {
        // The replication driver is co-located on the entity that holds the dynamic fragments. It may not
        // exist yet if the fragment opted into replication before the entity's replication was set up —
        // leave the dirty tag in place so we retry next tick.
        if (NOT InHandle.Has<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>())
        { return; }

        auto* Driver = InHandle.Get<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>().Get();

        if (ck::Is_NOT_Valid(Driver))
        { return; }

        for (const auto* Type : InReplicatedTypes.Get_Types())
        {
            if (NOT UCk_Utils_DynamicFragment_UE::Has_Fragment(InHandle, Type))
            { continue; }

            const auto* Payload = UCk_Utils_DynamicFragment_UE::TryGet_Fragment_TypeUnsafe(InHandle, Type);
            if (Payload == nullptr)
            { return; }

            Driver->SetFragmentData_Runtime(*Payload);
        }

        InHandle.Remove<FTag_DynamicFragment_MayRequireReplication>();
    }
}

// --------------------------------------------------------------------------------------------------------------------
