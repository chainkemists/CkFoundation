#include "CkDynamic_Processor.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkDynamic/CkDynamic_Utils.h"

#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Fragment.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_DynamicFragment_Replicate);
CK_REGISTER_PROCESSOR(ck::FProcessor_DynamicFragment_SyncReplication);

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

            const auto& Payload = UCk_Utils_DynamicFragment_UE::Get_Fragment_TypeUnsafe(InHandle, Type);
            Driver->SetFragmentData_Runtime(Payload);
        }

        InHandle.Remove<FTag_DynamicFragment_MayRequireReplication>();
    }

    // ---- SyncReplication (Client-side) ----

    auto
        FProcessor_DynamicFragment_SyncReplication::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_DynamicFragment_SyncReplication& InSync) const
        -> void
    {
        // Drain into a local copy and clear the fragment BEFORE applying — a bound OnRepNotify handler may
        // re-enter and mutate dynamic-fragment storage (including this sync fragment).
        const auto Payloads = InSync.Get_PendingPayloads();
        InHandle.Remove<FFragment_DynamicFragment_SyncReplication>();

        for (const auto& Payload : Payloads)
        {
            const auto* Type = Payload.GetScriptStruct();

            if (ck::Is_NOT_Valid(Type))
            { continue; }

            auto& Storage = UCk_Utils_DynamicFragment_UE::AddOrGet_Fragment_TypeUnsafe(InHandle, Type);
            Storage = Payload;

            auto Info = FCk_DynamicFragment_RepNotifyInfo{};
            Info.ChangedType = const_cast<UScriptStruct*>(Type);

            UUtils_Signal_DynamicFragment_OnRepNotify::Broadcast(InHandle, ck::MakePayload(InHandle, Info));
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
