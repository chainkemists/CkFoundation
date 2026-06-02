#include "CkFloatAttribute_Fragment.h"

#include "CkAttribute/CkAttribute_Log.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Utils.h"

#include "CkCore/Math/Arithmetic/CkArithmetic_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

#include "CkSnapshot/Context/CkSnapshot_FragmentRegistry.h"
#include "CkSnapshot/Archive/CkSnapshot_Archive_Writer.h"
#include "CkSnapshot/Archive/CkSnapshot_Archive_Reader.h"

// --------------------------------------------------------------------------------------------------------------------
// Tier-C SerializeSnapshot registration for the Float attribute family (TFragment_Attribute / -Modifier / -PreviousValues).
// CK_REGISTER_SNAPSHOTABLE pastes the type name into a generated identifier, so ck:: cannot be passed directly
// (the `::` breaks token-pasting). Each ck:: type is first hoisted to an unqualified file-scope alias.

using FSnap_FloatAttribute_Current = ck::FFragment_FloatAttribute_Current;
using FSnap_FloatAttribute_Min     = ck::FFragment_FloatAttribute_Min;
using FSnap_FloatAttribute_Max     = ck::FFragment_FloatAttribute_Max;

CK_REGISTER_SNAPSHOTABLE(FSnap_FloatAttribute_Current);
CK_REGISTER_SNAPSHOTABLE(FSnap_FloatAttribute_Min);
CK_REGISTER_SNAPSHOTABLE(FSnap_FloatAttribute_Max);

using FSnap_FloatAttributeModifier_Current = ck::FFragment_FloatAttributeModifier_Current;
using FSnap_FloatAttributeModifier_Min     = ck::FFragment_FloatAttributeModifier_Min;
using FSnap_FloatAttributeModifier_Max     = ck::FFragment_FloatAttributeModifier_Max;

CK_REGISTER_SNAPSHOTABLE(FSnap_FloatAttributeModifier_Current);
CK_REGISTER_SNAPSHOTABLE(FSnap_FloatAttributeModifier_Min);
CK_REGISTER_SNAPSHOTABLE(FSnap_FloatAttributeModifier_Max);

using FSnap_FloatAttribute_PreviousValues_Current = ck::TFragment_Attribute_PreviousValues<ck::FFragment_FloatAttribute_Current>;
using FSnap_FloatAttribute_PreviousValues_Min     = ck::TFragment_Attribute_PreviousValues<ck::FFragment_FloatAttribute_Min>;
using FSnap_FloatAttribute_PreviousValues_Max     = ck::TFragment_Attribute_PreviousValues<ck::FFragment_FloatAttribute_Max>;

CK_REGISTER_SNAPSHOTABLE(FSnap_FloatAttribute_PreviousValues_Current);
CK_REGISTER_SNAPSHOTABLE(FSnap_FloatAttribute_PreviousValues_Min);
CK_REGISTER_SNAPSHOTABLE(FSnap_FloatAttribute_PreviousValues_Max);

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Fragment_FloatAttribute_BaseFinal::
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
    ApplyReplicatedFloatAttributeEntry(
        FCk_Handle_FloatAttribute& InAttributeEntity,
        const FCk_Fragment_FloatAttribute_BaseFinal& InEntry)
    -> void
{
    UCk_Utils_FloatAttributeModifier_UE::Request_ClearAllModifiers(InAttributeEntity, InEntry.Get_Component());
    UCk_Utils_FloatAttribute_UE::Request_Override(InAttributeEntity, InEntry.Get_Base(), InEntry.Get_Component());

    auto AttributeModifier = UCk_Utils_FloatAttributeModifier_UE::TryGet(InAttributeEntity,
        ck::FAttributeModifier_ReplicationTags::Get_FinalTag(), InEntry.Get_Component());

    if (ck::Is_NOT_Valid(AttributeModifier))
    {
        UCk_Utils_FloatAttributeModifier_UE::Add_Revocable
        (
            InAttributeEntity,
            ck::FAttributeModifier_ReplicationTags::Get_FinalTag(),
            ECk_AttributeModifier_Operation::Add,
            FCk_Fragment_FloatAttributeModifier_ParamsData
            {
                InEntry.Get_Final() - InEntry.Get_Base(),
                InEntry.Get_Component()
            }
        );
    }
    else
    {
        UCk_Utils_FloatAttributeModifier_UE::Override(
            AttributeModifier, InEntry.Get_Final() - InEntry.Get_Base());
    }
}

static auto
    StashPendingFloatAttributeEntry(
        FCk_Handle& InOwnerEntity,
        const FCk_Fragment_FloatAttribute_BaseFinal& InEntry)
    -> void
{
    auto& Pending = InOwnerEntity.AddOrGet<ck::FFragment_FloatAttribute_PendingReplicationEntries>();

    auto Existing = Pending._PendingEntries.FindByPredicate([&](const FCk_Fragment_FloatAttribute_BaseFinal& InElement)
    {
        return InElement.Get_AttributeName() == InEntry.Get_AttributeName() && InElement.Get_Component() == InEntry.Get_Component();
    });

    if (ck::IsValid(Existing, ck::IsValid_Policy_NullptrOnly{}))
    { *Existing = InEntry; }
    else
    { Pending._PendingEntries.Emplace(InEntry); }
}

// --------------------------------------------------------------------------------------------------------------------
// Container-based replication handler for Float Attributes

static struct FFloatAttributeRepHandlerRegistrar
{
    FFloatAttributeRepHandlerRegistrar()
    {
        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
            []() -> UScriptStruct* { return FCk_RepData_FloatAttributes::StaticStruct(); },
            {
                .OnChange = [](FCk_Handle& Entity, const FInstancedStruct& New, const FInstancedStruct& Old)
                {
                    const auto& NewAttrs = New.Get<FCk_RepData_FloatAttributes>().Attributes;
                    const auto& OldAttrs = Old.Get<FCk_RepData_FloatAttributes>().Attributes;

                    for (auto Index = 0; Index < NewAttrs.Num(); ++Index)
                    {
                        const auto& Entry = NewAttrs[Index];

                        auto AttributeEntity = UCk_Utils_FloatAttribute_UE::TryGet(Entity, Entry.Get_AttributeName());
                        if (ck::Is_NOT_Valid(AttributeEntity))
                        {
                            StashPendingFloatAttributeEntry(Entity, Entry);
                            continue;
                        }

                        if (!OldAttrs.IsValidIndex(Index))
                        {
                            ck::attribute::Verbose(TEXT("Replicating FLOAT Attribute [{}] for the FIRST time to [{}|{}]"),
                                Entry.Get_AttributeName(), Entry.Get_Base(), Entry.Get_Final());

                            ApplyReplicatedFloatAttributeEntry(AttributeEntity, Entry);
                            continue;
                        }

                        if (OldAttrs[Index] != Entry)
                        {
                            ck::attribute::Verbose(TEXT("Replicating FLOAT Attribute [{}] and UPDATING it to [{}|{}]"),
                                Entry.Get_AttributeName(), Entry.Get_Base(), Entry.Get_Final());

                            ApplyReplicatedFloatAttributeEntry(AttributeEntity, Entry);
                            continue;
                        }
                    }
                },
                .OnAdd = [](FCk_Handle& Entity, const FInstancedStruct& Data)
                {
                    const auto& Attributes = Data.Get<FCk_RepData_FloatAttributes>().Attributes;

                    for (const auto& Entry : Attributes)
                    {
                        auto AttributeEntity = UCk_Utils_FloatAttribute_UE::TryGet(Entity, Entry.Get_AttributeName());
                        if (ck::Is_NOT_Valid(AttributeEntity))
                        {
                            StashPendingFloatAttributeEntry(Entity, Entry);
                            continue;
                        }

                        ck::attribute::Verbose(TEXT("Replicating FLOAT Attribute [{}] for the FIRST time to [{}|{}]"),
                            Entry.Get_AttributeName(), Entry.Get_Base(), Entry.Get_Final());

                        ApplyReplicatedFloatAttributeEntry(AttributeEntity, Entry);
                    }
                }
            });
    }
} GFloatAttributeRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------