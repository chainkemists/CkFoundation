#include "CkFloatAttribute_Fragment.h"

#include "CkAttribute/CkAttribute_Log.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Utils.h"

#include "CkCore/Math/Arithmetic/CkArithmetic_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

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