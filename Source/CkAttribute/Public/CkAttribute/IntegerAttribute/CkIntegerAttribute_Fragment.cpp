#include "CkIntegerAttribute_Fragment.h"

#include "CkAttribute/CkAttribute_Log.h"
#include "CkAttribute/IntegerAttribute/CkIntegerAttribute_Utils.h"

#include "CkCore/Math/Arithmetic/CkArithmetic_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"
#include "CkAttribute/CkAttribute_RestorePersistence.h" // Register_* + shared attribute Produce/HydrationApply
#include "CkAttribute/CkAttribute_RefillPersistence.h"  // shared refill run-state Produce + HydrationApply helpers

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Fragment_IntegerAttribute_BaseFinal::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return _AttributeName == InOther.Get_AttributeName() &&
        _Component == InOther.Get_Component() &&
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

// --------------------------------------------------------------------------------------------------------------------

static struct FIntegerAttributeRepHandlerRegistrar
{
    FIntegerAttributeRepHandlerRegistrar()
    {
        FCk_PersistenceHandlerRegistry::Register_NetAndSave_SplitApply<FCk_RepData_IntegerAttributes>({
                .Produce = &ck::attribute_restore::Produce<ck::TFragment_IntegerAttribute, FCk_RepData_IntegerAttributes>,
                .NetApply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& Old) -> ECk_Persistence_ApplyResult
                {
                    const auto& NewAttrs = New.Get<FCk_RepData_IntegerAttributes>().Attributes;
                    const auto* OldAttrs = Old.IsSet()
                        ? &Old.GetValue().Get<FCk_RepData_IntegerAttributes>().Attributes
                        : nullptr;

                    auto Result = ECk_Persistence_ApplyResult::Applied;

                    for (auto Index = 0; Index < NewAttrs.Num(); ++Index)
                    {
                        const auto& Entry = NewAttrs[Index];

                        auto AttributeEntity = UCk_Utils_IntegerAttribute_UE::TryGet(Entity, Entry.Get_AttributeName());
                        if (ck::Is_NOT_Valid(AttributeEntity))
                        {
                            Result = ECk_Persistence_ApplyResult::NotReady;
                            continue;
                        }

                        const auto UnchangedSinceLastApply = OldAttrs != nullptr &&
                            OldAttrs->IsValidIndex(Index) && (*OldAttrs)[Index] == Entry;
                        if (UnchangedSinceLastApply)
                        { continue; }

                        const auto CurrentValues = FCk_Fragment_IntegerAttribute_BaseFinal
                        {
                            Entry.Get_AttributeName(),
                            UCk_Utils_IntegerAttribute_UE::Get_BaseValue(AttributeEntity, Entry.Get_Component()),
                            UCk_Utils_IntegerAttribute_UE::Get_FinalValue(AttributeEntity, Entry.Get_Component()),
                            Entry.Get_Component()
                        };
                        if (CurrentValues == Entry)
                        { continue; }

                        ck::attribute::Verbose(TEXT("Replicating INTEGER Attribute [{}] to [{}|{}]"),
                            Entry.Get_AttributeName(), Entry.Get_Base(), Entry.Get_Final());

                        ApplyReplicatedIntegerAttributeEntry(AttributeEntity, Entry);
                    }

                    return Result;
                },
                .HydrationApply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_Persistence_ApplyResult
                {
                    return ck::attribute_restore::HydrationApply<ck::TFragment_IntegerAttribute, FCk_RepData_IntegerAttributes, UCk_Utils_IntegerAttribute_UE>(
                        Entity, New, &ApplyReplicatedIntegerAttributeEntry);
                }});
    }
} GIntegerAttributeRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------

static struct FIntegerAttributeRefillRepHandlerRegistrar
{
    FIntegerAttributeRefillRepHandlerRegistrar()
    {
        FCk_PersistenceHandlerRegistry::Register_SaveOnly<FCk_SaveData_IntegerAttributeRefill>({
                .Produce = &ck::attribute_refill_restore::Produce<
                    ck::TFragment_IntegerAttribute, FCk_Handle_IntegerAttributeRefill, UCk_Utils_IntegerAttributeRefill_UE, FCk_SaveData_IntegerAttributeRefill>,
                .HydrationApply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_Persistence_ApplyResult
                {
                    return ck::attribute_refill_restore::HydrationApply<
                        ck::TFragment_IntegerAttribute, FCk_Handle_IntegerAttributeRefill, UCk_Utils_IntegerAttributeRefill_UE, FCk_SaveData_IntegerAttributeRefill>(
                            Entity, New);
                }});
    }
} GIntegerAttributeRefillRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------