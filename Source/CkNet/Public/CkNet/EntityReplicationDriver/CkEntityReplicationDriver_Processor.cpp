#include "CkEntityReplicationDriver_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkNet/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_ReplicationDriver_ReplicateEntityScript::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FCk_Request_EntityScript_Replicate& InRequest) const
        -> void
    {
        UCk_Utils_EntityReplicationDriver_UE::Add(InHandle);

        FCk_Handle ReplicatedOwner = InRequest.Get_Owner();

        if (UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(ReplicatedOwner))
        {
            const auto& ReplicationDriver = ReplicatedOwner.Get<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>();

            ReplicationDriver->Set_ExpectedNumberOfDependentReplicationDrivers(
                ReplicationDriver->Get_ExpectedNumberOfDependentReplicationDrivers() + 1);

            UCk_Utils_EntityReplicationDriver_UE::Request_Replicate(InHandle, ReplicatedOwner,
                InRequest.Get_EntityScriptClass());
        }

        InHandle.Remove<MarkedDirtyBy>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ReplicationDriver_FireOnDependentReplicationComplete::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Signal_OnDependentsReplicationComplete&) const
        -> void
    {
        InHandle.Remove<MarkedDirtyBy>();
        UUtils_Signal_OnDependentsReplicationComplete::Broadcast(InHandle, ck::MakePayload(InHandle));
    }
}

// --------------------------------------------------------------------------------------------------------------------

