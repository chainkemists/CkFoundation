#include "CkByteAttribute_Fragment.h"

#include "CkAttribute/CkAttribute_Log.h"
#include "CkAttribute/ByteAttribute/CkByteAttribute_Utils.h"

#include "CkCore/Math/Arithmetic/CkArithmetic_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

#include "CkSnapshot/Context/CkSnapshot_FragmentRegistry.h"
#include "CkSnapshot/Archive/CkSnapshot_Archive_Writer.h"
#include "CkSnapshot/Archive/CkSnapshot_Archive_Reader.h"

// --------------------------------------------------------------------------------------------------------------------
// Tier-C SerializeSnapshot registration for the Byte attribute family. ck:: types are hoisted to unqualified
// file-scope aliases because CK_REGISTER_SNAPSHOTABLE token-pastes the type name (the `::` cannot be pasted).

using FSnap_ByteAttribute_Current = ck::FFragment_ByteAttribute_Current;
using FSnap_ByteAttribute_Min     = ck::FFragment_ByteAttribute_Min;
using FSnap_ByteAttribute_Max     = ck::FFragment_ByteAttribute_Max;

CK_REGISTER_SNAPSHOTABLE(FSnap_ByteAttribute_Current);
CK_REGISTER_SNAPSHOTABLE(FSnap_ByteAttribute_Min);
CK_REGISTER_SNAPSHOTABLE(FSnap_ByteAttribute_Max);

using FSnap_ByteAttributeModifier_Current = ck::FFragment_ByteAttributeModifier_Current;
using FSnap_ByteAttributeModifier_Min     = ck::FFragment_ByteAttributeModifier_Min;
using FSnap_ByteAttributeModifier_Max     = ck::FFragment_ByteAttributeModifier_Max;

CK_REGISTER_SNAPSHOTABLE(FSnap_ByteAttributeModifier_Current);
CK_REGISTER_SNAPSHOTABLE(FSnap_ByteAttributeModifier_Min);
CK_REGISTER_SNAPSHOTABLE(FSnap_ByteAttributeModifier_Max);

using FSnap_ByteAttribute_PreviousValues_Current = ck::TFragment_Attribute_PreviousValues<ck::FFragment_ByteAttribute_Current>;
using FSnap_ByteAttribute_PreviousValues_Min     = ck::TFragment_Attribute_PreviousValues<ck::FFragment_ByteAttribute_Min>;
using FSnap_ByteAttribute_PreviousValues_Max     = ck::TFragment_Attribute_PreviousValues<ck::FFragment_ByteAttribute_Max>;

CK_REGISTER_SNAPSHOTABLE(FSnap_ByteAttribute_PreviousValues_Current);
CK_REGISTER_SNAPSHOTABLE(FSnap_ByteAttribute_PreviousValues_Min);
CK_REGISTER_SNAPSHOTABLE(FSnap_ByteAttribute_PreviousValues_Max);

using FSnap_RecordOfByteAttributes = ck::FFragment_RecordOfByteAttributes;
CK_REGISTER_SNAPSHOTABLE(FSnap_RecordOfByteAttributes);

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Fragment_ByteAttribute_BaseFinal::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return _AttributeName == InOther.Get_AttributeName() &&
        _Component == Get_Component() &&
        UCk_Utils_Arithmetic_UE::Get_IsNearlyEqual(_Base, InOther.Get_Base()) &&
        UCk_Utils_Arithmetic_UE::Get_IsNearlyEqual(_Final, InOther.Get_Final());
}

// --------------------------------------------------------------------------------------------------------------------

static auto
    ApplyReplicatedByteAttributeEntry(
        FCk_Handle_ByteAttribute& InAttributeEntity,
        const FCk_Fragment_ByteAttribute_BaseFinal& InEntry)
    -> void
{
    UCk_Utils_ByteAttributeModifier_UE::Request_ClearAllModifiers(InAttributeEntity, InEntry.Get_Component());
    UCk_Utils_ByteAttribute_UE::Request_Override(InAttributeEntity, InEntry.Get_Base(), InEntry.Get_Component());

    auto AttributeModifier = UCk_Utils_ByteAttributeModifier_UE::TryGet(InAttributeEntity,
        ck::FAttributeModifier_ReplicationTags::Get_FinalTag(), InEntry.Get_Component());

    if (ck::Is_NOT_Valid(AttributeModifier))
    {
        const auto Difference = InEntry.Get_Final() - InEntry.Get_Base();

        UCk_Utils_ByteAttributeModifier_UE::Add_Revocable
        (
            InAttributeEntity,
            ck::FAttributeModifier_ReplicationTags::Get_FinalTag(),
            Difference >= 0 ? ECk_AttributeModifier_Operation::Add : ECk_AttributeModifier_Operation::Subtract,
            FCk_Fragment_ByteAttributeModifier_ParamsData
            {
                static_cast<uint8>(std::abs(Difference)),
                InEntry.Get_Component()
            }
        );
    }
    else
    {
        UCk_Utils_ByteAttributeModifier_UE::Override(
            AttributeModifier, InEntry.Get_Final() - InEntry.Get_Base());
    }
}

static auto
    StashPendingByteAttributeEntry(
        FCk_Handle& InOwnerEntity,
        const FCk_Fragment_ByteAttribute_BaseFinal& InEntry)
    -> void
{
    auto& Pending = InOwnerEntity.AddOrGet<ck::FFragment_ByteAttribute_PendingReplicationEntries>();

    auto Existing = Pending._PendingEntries.FindByPredicate([&](const FCk_Fragment_ByteAttribute_BaseFinal& InElement)
    {
        return InElement.Get_AttributeName() == InEntry.Get_AttributeName() && InElement.Get_Component() == InEntry.Get_Component();
    });

    if (ck::IsValid(Existing, ck::IsValid_Policy_NullptrOnly{}))
    { *Existing = InEntry; }
    else
    { Pending._PendingEntries.Emplace(InEntry); }
}

// --------------------------------------------------------------------------------------------------------------------
// Container-based replication handler for Byte Attributes

static struct FByteAttributeRepHandlerRegistrar
{
    FByteAttributeRepHandlerRegistrar()
    {
        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
            []() -> UScriptStruct* { return FCk_RepData_ByteAttributes::StaticStruct(); },
            {
                .OnChange = [](FCk_Handle& Entity, const FInstancedStruct& New, const FInstancedStruct& Old)
                {
                    const auto& NewAttrs = New.Get<FCk_RepData_ByteAttributes>().Attributes;
                    const auto& OldAttrs = Old.Get<FCk_RepData_ByteAttributes>().Attributes;

                    for (auto Index = 0; Index < NewAttrs.Num(); ++Index)
                    {
                        const auto& Entry = NewAttrs[Index];

                        auto AttributeEntity = UCk_Utils_ByteAttribute_UE::TryGet(Entity, Entry.Get_AttributeName());
                        if (ck::Is_NOT_Valid(AttributeEntity))
                        {
                            StashPendingByteAttributeEntry(Entity, Entry);
                            continue;
                        }

                        if (!OldAttrs.IsValidIndex(Index))
                        {
                            ck::attribute::Verbose(TEXT("Replicating BYTE Attribute [{}] for the FIRST time to [{}|{}]"),
                                Entry.Get_AttributeName(), Entry.Get_Base(), Entry.Get_Final());

                            ApplyReplicatedByteAttributeEntry(AttributeEntity, Entry);
                            continue;
                        }

                        if (OldAttrs[Index] != Entry)
                        {
                            ck::attribute::Verbose(TEXT("Replicating BYTE Attribute [{}] and UPDATING it to [{}|{}]"),
                                Entry.Get_AttributeName(), Entry.Get_Base(), Entry.Get_Final());

                            ApplyReplicatedByteAttributeEntry(AttributeEntity, Entry);
                            continue;
                        }
                    }
                },
                .OnAdd = [](FCk_Handle& Entity, const FInstancedStruct& Data)
                {
                    const auto& Attributes = Data.Get<FCk_RepData_ByteAttributes>().Attributes;

                    for (const auto& Entry : Attributes)
                    {
                        auto AttributeEntity = UCk_Utils_ByteAttribute_UE::TryGet(Entity, Entry.Get_AttributeName());
                        if (ck::Is_NOT_Valid(AttributeEntity))
                        {
                            StashPendingByteAttributeEntry(Entity, Entry);
                            continue;
                        }

                        ck::attribute::Verbose(TEXT("Replicating BYTE Attribute [{}] for the FIRST time to [{}|{}]"),
                            Entry.Get_AttributeName(), Entry.Get_Base(), Entry.Get_Final());

                        ApplyReplicatedByteAttributeEntry(AttributeEntity, Entry);
                    }
                }
            });
    }
} GByteAttributeRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
