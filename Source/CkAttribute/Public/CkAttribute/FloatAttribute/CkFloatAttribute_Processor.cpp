#include "CkFloatAttribute_Processor.h"

#include "CkAttribute/CkAttribute_Log.h"
#include "CkAttribute/CkAttribute_Processor.inl.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_FloatAttribute_RecomputeAll);
CK_REGISTER_PROCESSOR(ck::FProcessor_FloatAttributeModifier_ComputeAll);
CK_REGISTER_PROCESSOR(ck::FProcessor_FloatAttribute_MinMaxClamp);
CK_REGISTER_PROCESSOR(ck::FProcessor_FloatAttribute_FireSignals);
CK_REGISTER_PROCESSOR(ck::FProcessor_FloatAttributeModifier_EndPlayAll);
CK_REGISTER_PROCESSOR(ck::FProcessor_FloatAttribute_Replicate);
CK_REGISTER_PROCESSOR(ck::FProcessor_FloatAttribute_Refill);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_FloatAttribute_RetryPendingReplication::
        ForEachEntity(
            TimeType InDeltaT,
            FCk_Handle InHandle,
            FFragment_FloatAttribute_PendingReplicationEntries& InPending) const
        -> void
    {
        for (auto Index = InPending._PendingEntries.Num() - 1; Index >= 0; --Index)
        {
            const auto& Entry = InPending._PendingEntries[Index];

            auto AttributeEntity = UCk_Utils_FloatAttribute_UE::TryGet(InHandle, Entry.Get_AttributeName());
            if (ck::Is_NOT_Valid(AttributeEntity))
            { continue; }

            ck::attribute::Verbose(TEXT("Retrying pending replication of FLOAT Attribute [{}] to [{}|{}]"),
                Entry.Get_AttributeName(), Entry.Get_Base(), Entry.Get_Final());

            UCk_Utils_FloatAttributeModifier_UE::Request_ClearAllModifiers(AttributeEntity, Entry.Get_Component());
            UCk_Utils_FloatAttribute_UE::Request_Override(AttributeEntity, Entry.Get_Base(), Entry.Get_Component());

            UCk_Utils_FloatAttributeModifier_UE::Add_Revocable
            (
                AttributeEntity,
                ck::FAttributeModifier_ReplicationTags::Get_FinalTag(),
                ECk_AttributeModifier_Operation::Add,
                FCk_Fragment_FloatAttributeModifier_ParamsData
                {
                    Entry.Get_Final() - Entry.Get_Base(),
                    Entry.Get_Component()
                }
            );

            InPending._PendingEntries.RemoveAt(Index);
        }

        if (InPending._PendingEntries.IsEmpty())
        { InHandle.Remove<FFragment_FloatAttribute_PendingReplicationEntries>(); }
    }
}

// --------------------------------------------------------------------------------------------------------------------
