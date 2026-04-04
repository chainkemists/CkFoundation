#include "CkIntegerAttribute_Fragment.h"

#include "CkAttribute/CkAttribute_Log.h"
#include "CkAttribute/IntegerAttribute/CkIntegerAttribute_Utils.h"

#include "CkCore/Math/Arithmetic/CkArithmetic_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Fragment_IntegerAttribute_BaseFinal::
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
    ApplyReplicatedIntegerAttributeEntry(
        FCk_Handle_IntegerAttribute& InAttributeEntity,
        const FCk_Fragment_IntegerAttribute_BaseFinal& InEntry)
    -> void
{
    UCk_Utils_IntegerAttributeModifier_UE::Request_ClearAllModifiers(InAttributeEntity, InEntry.Get_Component());
    UCk_Utils_IntegerAttribute_UE::Request_Override(InAttributeEntity, InEntry.Get_Base(), InEntry.Get_Component());

    auto AttributeModifier = UCk_Utils_IntegerAttributeModifier_UE::TryGet(InAttributeEntity,
        ck::FAttributeModifier_ReplicationTags::Get_FinalTag(), InEntry.Get_Component());

    if (ck::Is_NOT_Valid(AttributeModifier))
    {
        UCk_Utils_IntegerAttributeModifier_UE::Add_Revocable
        (
            InAttributeEntity,
            ck::FAttributeModifier_ReplicationTags::Get_FinalTag(),
            ECk_AttributeModifier_Operation::Add,
            FCk_Fragment_IntegerAttributeModifier_ParamsData
            {
                InEntry.Get_Final() - InEntry.Get_Base(),
                InEntry.Get_Component()
            }
        );
    }
    else
    {
        UCk_Utils_IntegerAttributeModifier_UE::Override(
            AttributeModifier, InEntry.Get_Final() - InEntry.Get_Base());
    }
}

static auto
    StashPendingIntegerAttributeEntry(
        FCk_Handle& InOwnerEntity,
        const FCk_Fragment_IntegerAttribute_BaseFinal& InEntry)
    -> void
{
    auto& Pending = InOwnerEntity.AddOrGet<ck::FFragment_IntegerAttribute_PendingReplicationEntries>();

    auto Existing = Pending._PendingEntries.FindByPredicate([&](const FCk_Fragment_IntegerAttribute_BaseFinal& InElement)
    {
        return InElement.Get_AttributeName() == InEntry.Get_AttributeName() && InElement.Get_Component() == InEntry.Get_Component();
    });

    if (ck::IsValid(Existing, ck::IsValid_Policy_NullptrOnly{}))
    { *Existing = InEntry; }
    else
    { Pending._PendingEntries.Emplace(InEntry); }
}

// --------------------------------------------------------------------------------------------------------------------
// Container-based replication handler for Integer Attributes

static struct FIntegerAttributeRepHandlerRegistrar
{
    FIntegerAttributeRepHandlerRegistrar()
    {
        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
            []() -> UScriptStruct* { return FCk_RepData_IntegerAttributes::StaticStruct(); },
            {
                .OnChange = [](FCk_Handle& Entity, const FInstancedStruct& New, const FInstancedStruct& Old)
                {
                    const auto& NewAttrs = New.Get<FCk_RepData_IntegerAttributes>().Attributes;
                    const auto& OldAttrs = Old.Get<FCk_RepData_IntegerAttributes>().Attributes;

                    for (auto Index = 0; Index < NewAttrs.Num(); ++Index)
                    {
                        const auto& Entry = NewAttrs[Index];

                        auto AttributeEntity = UCk_Utils_IntegerAttribute_UE::TryGet(Entity, Entry.Get_AttributeName());
                        if (ck::Is_NOT_Valid(AttributeEntity))
                        {
                            StashPendingIntegerAttributeEntry(Entity, Entry);
                            continue;
                        }

                        if (!OldAttrs.IsValidIndex(Index))
                        {
                            ck::attribute::Verbose(TEXT("Replicating INTEGER Attribute [{}] for the FIRST time to [{}|{}]"),
                                Entry.Get_AttributeName(), Entry.Get_Base(), Entry.Get_Final());

                            ApplyReplicatedIntegerAttributeEntry(AttributeEntity, Entry);
                            continue;
                        }

                        if (OldAttrs[Index] != Entry)
                        {
                            ck::attribute::Verbose(TEXT("Replicating INTEGER Attribute [{}] and UPDATING it to [{}|{}]"),
                                Entry.Get_AttributeName(), Entry.Get_Base(), Entry.Get_Final());

                            ApplyReplicatedIntegerAttributeEntry(AttributeEntity, Entry);
                            continue;
                        }
                    }
                },
                .OnAdd = [](FCk_Handle& Entity, const FInstancedStruct& Data)
                {
                    const auto& Attributes = Data.Get<FCk_RepData_IntegerAttributes>().Attributes;

                    for (const auto& Entry : Attributes)
                    {
                        auto AttributeEntity = UCk_Utils_IntegerAttribute_UE::TryGet(Entity, Entry.Get_AttributeName());
                        if (ck::Is_NOT_Valid(AttributeEntity))
                        {
                            StashPendingIntegerAttributeEntry(Entity, Entry);
                            continue;
                        }

                        ck::attribute::Verbose(TEXT("Replicating INTEGER Attribute [{}] for the FIRST time to [{}|{}]"),
                            Entry.Get_AttributeName(), Entry.Get_Base(), Entry.Get_Final());

                        ApplyReplicatedIntegerAttributeEntry(AttributeEntity, Entry);
                    }
                }
            });
    }
} GIntegerAttributeRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------