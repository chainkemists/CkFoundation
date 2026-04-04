#include "CkByteAttribute_Processor.h"

#include "CkAttribute/CkAttribute_Log.h"
#include "CkAttribute/CkAttribute_Processor.inl.h"
#include "CkAttribute/ByteAttribute/CkByteAttribute_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_ByteAttribute_RecomputeAll);
CK_REGISTER_PROCESSOR(ck::FProcessor_ByteAttributeModifier_ComputeAll);
CK_REGISTER_PROCESSOR(ck::FProcessor_ByteAttribute_MinMaxClamp);
CK_REGISTER_PROCESSOR(ck::FProcessor_ByteAttribute_FireSignals);
CK_REGISTER_PROCESSOR(ck::FProcessor_ByteAttributeModifier_EndPlayAll);
CK_REGISTER_PROCESSOR(ck::FProcessor_ByteAttribute_Replicate);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_ByteAttribute_RetryPendingReplication::
        ForEachEntity(
            TimeType InDeltaT,
            FCk_Handle InHandle,
            FFragment_ByteAttribute_PendingReplicationEntries& InPending) const
        -> void
    {
        for (auto Index = InPending._PendingEntries.Num() - 1; Index >= 0; --Index)
        {
            const auto& Entry = InPending._PendingEntries[Index];

            auto AttributeEntity = UCk_Utils_ByteAttribute_UE::TryGet(InHandle, Entry.Get_AttributeName());
            if (ck::Is_NOT_Valid(AttributeEntity))
            { continue; }

            ck::attribute::Verbose(TEXT("Retrying pending replication of BYTE Attribute [{}] to [{}|{}]"),
                Entry.Get_AttributeName(), Entry.Get_Base(), Entry.Get_Final());

            UCk_Utils_ByteAttributeModifier_UE::Request_ClearAllModifiers(AttributeEntity, Entry.Get_Component());
            UCk_Utils_ByteAttribute_UE::Request_Override(AttributeEntity, Entry.Get_Base(), Entry.Get_Component());

            const auto Difference = Entry.Get_Final() - Entry.Get_Base();

            UCk_Utils_ByteAttributeModifier_UE::Add_Revocable
            (
                AttributeEntity,
                ck::FAttributeModifier_ReplicationTags::Get_FinalTag(),
                Difference >= 0 ? ECk_AttributeModifier_Operation::Add : ECk_AttributeModifier_Operation::Subtract,
                FCk_Fragment_ByteAttributeModifier_ParamsData
                {
                    static_cast<uint8>(std::abs(Difference)),
                    Entry.Get_Component()
                }
            );

            InPending._PendingEntries.RemoveAt(Index);
        }

        if (InPending._PendingEntries.IsEmpty())
        { InHandle.Remove<FFragment_ByteAttribute_PendingReplicationEntries>(); }
    }
}

// --------------------------------------------------------------------------------------------------------------------
