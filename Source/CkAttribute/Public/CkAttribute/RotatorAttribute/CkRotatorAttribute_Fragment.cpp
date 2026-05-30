#include "CkRotatorAttribute_Fragment.h"

#include "CkAttribute/CkAttribute_Log.h"
#include "CkAttribute/RotatorAttribute/CkRotatorAttribute_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Fragment_RotatorAttribute_BaseFinal::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return _AttributeName == InOther.Get_AttributeName() &&
        _Component == Get_Component() &&
        _Base.Equals(InOther.Get_Base()) &&
        _Final.Equals(InOther.Get_Final());
}

// --------------------------------------------------------------------------------------------------------------------

static auto
    ApplyReplicatedRotatorAttributeEntry(
        FCk_Handle_RotatorAttribute& InAttributeEntity,
        const FCk_Fragment_RotatorAttribute_BaseFinal& InEntry)
    -> void
{
    UCk_Utils_RotatorAttributeModifier_UE::Request_ClearAllModifiers(InAttributeEntity, InEntry.Get_Component());
    UCk_Utils_RotatorAttribute_UE::Request_Override(InAttributeEntity, InEntry.Get_Base(), InEntry.Get_Component());

    auto AttributeModifier = UCk_Utils_RotatorAttributeModifier_UE::TryGet(InAttributeEntity,
        ck::FAttributeModifier_ReplicationTags::Get_FinalTag(), InEntry.Get_Component());

    if (ck::Is_NOT_Valid(AttributeModifier))
    {
        UCk_Utils_RotatorAttributeModifier_UE::Add_Revocable
        (
            InAttributeEntity,
            ck::FAttributeModifier_ReplicationTags::Get_FinalTag(),
            ECk_AttributeModifier_Operation::Add,
            FCk_Fragment_RotatorAttributeModifier_ParamsData
            {
                InEntry.Get_Final() - InEntry.Get_Base(),
                InEntry.Get_Component()
            }
        );
    }
    else
    {
        UCk_Utils_RotatorAttributeModifier_UE::Override(
            AttributeModifier, InEntry.Get_Final() - InEntry.Get_Base());
    }
}

static auto
    StashPendingRotatorAttributeEntry(
        FCk_Handle& InOwnerEntity,
        const FCk_Fragment_RotatorAttribute_BaseFinal& InEntry)
    -> void
{
    auto& Pending = InOwnerEntity.AddOrGet<ck::FFragment_RotatorAttribute_PendingReplicationEntries>();

    auto Existing = Pending._PendingEntries.FindByPredicate([&](const FCk_Fragment_RotatorAttribute_BaseFinal& InElement)
    {
        return InElement.Get_AttributeName() == InEntry.Get_AttributeName() && InElement.Get_Component() == InEntry.Get_Component();
    });

    if (ck::IsValid(Existing, ck::IsValid_Policy_NullptrOnly{}))
    { *Existing = InEntry; }
    else
    { Pending._PendingEntries.Emplace(InEntry); }
}

// --------------------------------------------------------------------------------------------------------------------
// Container-based replication handler for Rotator Attributes

static struct FRotatorAttributeRepHandlerRegistrar
{
    FRotatorAttributeRepHandlerRegistrar()
    {
        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
            []() -> UScriptStruct* { return FCk_RepData_RotatorAttributes::StaticStruct(); },
            {
                .OnChange = [](FCk_Handle& Entity, const FInstancedStruct& New, const FInstancedStruct& Old)
                {
                    const auto& NewAttrs = New.Get<FCk_RepData_RotatorAttributes>().Attributes;
                    const auto& OldAttrs = Old.Get<FCk_RepData_RotatorAttributes>().Attributes;

                    for (auto Index = 0; Index < NewAttrs.Num(); ++Index)
                    {
                        const auto& Entry = NewAttrs[Index];

                        auto AttributeEntity = UCk_Utils_RotatorAttribute_UE::TryGet(Entity, Entry.Get_AttributeName());
                        if (ck::Is_NOT_Valid(AttributeEntity))
                        {
                            StashPendingRotatorAttributeEntry(Entity, Entry);
                            continue;
                        }

                        if (!OldAttrs.IsValidIndex(Index))
                        {
                            ck::attribute::Verbose(TEXT("Replicating ROTATOR Attribute [{}] for the FIRST time to [{}|{}]"),
                                Entry.Get_AttributeName(), Entry.Get_Base(), Entry.Get_Final());

                            ApplyReplicatedRotatorAttributeEntry(AttributeEntity, Entry);
                            continue;
                        }

                        if (OldAttrs[Index] != Entry)
                        {
                            ck::attribute::Verbose(TEXT("Replicating ROTATOR Attribute [{}] and UPDATING it to [{}|{}]"),
                                Entry.Get_AttributeName(), Entry.Get_Base(), Entry.Get_Final());

                            ApplyReplicatedRotatorAttributeEntry(AttributeEntity, Entry);
                            continue;
                        }
                    }
                },
                .OnAdd = [](FCk_Handle& Entity, const FInstancedStruct& Data)
                {
                    const auto& Attributes = Data.Get<FCk_RepData_RotatorAttributes>().Attributes;

                    for (const auto& Entry : Attributes)
                    {
                        auto AttributeEntity = UCk_Utils_RotatorAttribute_UE::TryGet(Entity, Entry.Get_AttributeName());
                        if (ck::Is_NOT_Valid(AttributeEntity))
                        {
                            StashPendingRotatorAttributeEntry(Entity, Entry);
                            continue;
                        }

                        ck::attribute::Verbose(TEXT("Replicating ROTATOR Attribute [{}] for the FIRST time to [{}|{}]"),
                            Entry.Get_AttributeName(), Entry.Get_Base(), Entry.Get_Final());

                        ApplyReplicatedRotatorAttributeEntry(AttributeEntity, Entry);
                    }
                }
            });
    }
} GRotatorAttributeRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
