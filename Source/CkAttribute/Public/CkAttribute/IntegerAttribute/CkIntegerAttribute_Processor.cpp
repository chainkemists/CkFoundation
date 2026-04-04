#include "CkIntegerAttribute_Processor.h"

#include "CkAttribute/CkAttribute_Log.h"
#include "CkAttribute/CkAttribute_Processor.inl.h"
#include "CkAttribute/IntegerAttribute/CkIntegerAttribute_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_IntegerAttribute_RecomputeAll);
CK_REGISTER_PROCESSOR(ck::FProcessor_IntegerAttributeModifier_ComputeAll);
CK_REGISTER_PROCESSOR(ck::FProcessor_IntegerAttribute_MinMaxClamp);
CK_REGISTER_PROCESSOR(ck::FProcessor_IntegerAttribute_FireSignals);
CK_REGISTER_PROCESSOR(ck::FProcessor_IntegerAttributeModifier_EndPlayAll);
CK_REGISTER_PROCESSOR(ck::FProcessor_IntegerAttribute_Replicate);
CK_REGISTER_PROCESSOR(ck::FProcessor_IntegerAttribute_Refill);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_IntegerAttribute_RetryPendingReplication::
        ForEachEntity(
            TimeType InDeltaT,
            FCk_Handle InHandle,
            FFragment_IntegerAttribute_PendingReplicationEntries& InPending) const
        -> void
    {
        for (auto Index = InPending._PendingEntries.Num() - 1; Index >= 0; --Index)
        {
            const auto& Entry = InPending._PendingEntries[Index];

            auto AttributeEntity = UCk_Utils_IntegerAttribute_UE::TryGet(InHandle, Entry.Get_AttributeName());
            if (ck::Is_NOT_Valid(AttributeEntity))
            { continue; }

            ck::attribute::Verbose(TEXT("Retrying pending replication of INTEGER Attribute [{}] to [{}|{}]"),
                Entry.Get_AttributeName(), Entry.Get_Base(), Entry.Get_Final());

            UCk_Utils_IntegerAttributeModifier_UE::Request_ClearAllModifiers(AttributeEntity, Entry.Get_Component());
            UCk_Utils_IntegerAttribute_UE::Request_Override(AttributeEntity, Entry.Get_Base(), Entry.Get_Component());

            UCk_Utils_IntegerAttributeModifier_UE::Add_Revocable
            (
                AttributeEntity,
                ck::FAttributeModifier_ReplicationTags::Get_FinalTag(),
                ECk_AttributeModifier_Operation::Add,
                FCk_Fragment_IntegerAttributeModifier_ParamsData
                {
                    Entry.Get_Final() - Entry.Get_Base(),
                    Entry.Get_Component()
                }
            );

            InPending._PendingEntries.RemoveAt(Index);
        }

        if (InPending._PendingEntries.IsEmpty())
        { InHandle.Remove<FFragment_IntegerAttribute_PendingReplicationEntries>(); }
    }
}

// --------------------------------------------------------------------------------------------------------------------
