#include "CkVectorAttribute_Fragment.h"

#include "CkAttribute/CkAttribute_Log.h"
#include "CkAttribute/VectorAttribute/CkVectorAttribute_Utils.h"

#include "CkCore/Math/Arithmetic/CkArithmetic_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"
#include "CkAttribute/CkAttribute_RestorePersistence.h" // Register_* + shared attribute Produce/HydrationApply

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Fragment_VectorAttribute_BaseFinal::
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
    ApplyReplicatedVectorAttributeEntry(
        FCk_Handle_VectorAttribute& InAttributeEntity,
        const FCk_Fragment_VectorAttribute_BaseFinal& InEntry)
    -> void
{
    UCk_Utils_VectorAttributeModifier_UE::Request_ClearAllModifiers(InAttributeEntity, InEntry.Get_Component(), {});
    UCk_Utils_VectorAttribute_UE::Request_Override(InAttributeEntity, InEntry.Get_Base(), InEntry.Get_Component(), {});

    auto AttributeModifier = UCk_Utils_VectorAttributeModifier_UE::TryGet(InAttributeEntity,
        ck::FAttributeModifier_ReplicationTags::Get_FinalTag(), InEntry.Get_Component());

    if (ck::Is_NOT_Valid(AttributeModifier))
    {
        UCk_Utils_VectorAttributeModifier_UE::Add_Revocable
        (
            InAttributeEntity,
            ck::FAttributeModifier_ReplicationTags::Get_FinalTag(),
            ECk_AttributeModifier_Operation::Add,
            FCk_Fragment_VectorAttributeModifier_ParamsData
            {
                InEntry.Get_Final() - InEntry.Get_Base(),
                InEntry.Get_Component()
            }
        );
    }
    else
    {
        UCk_Utils_VectorAttributeModifier_UE::Override(
            AttributeModifier, InEntry.Get_Final() - InEntry.Get_Base());
    }
}

// --------------------------------------------------------------------------------------------------------------------

static struct FVectorAttributeRepHandlerRegistrar
{
    FVectorAttributeRepHandlerRegistrar()
    {
        FCk_PersistenceHandlerRegistry::Register_NetAndSave_SplitApply<FCk_RepData_VectorAttributes>({
                .Produce = &ck::attribute_restore::Produce<ck::TFragment_VectorAttribute, FCk_RepData_VectorAttributes>,
                .NetApply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& Old) -> ECk_Persistence_ApplyResult
                {
                    const auto& NewAttrs = New.Get<FCk_RepData_VectorAttributes>().Attributes;
                    const auto* OldAttrs = Old.IsSet()
                        ? &Old.GetValue().Get<FCk_RepData_VectorAttributes>().Attributes
                        : nullptr;

                    auto Result = ECk_Persistence_ApplyResult::Applied;

                    for (auto Index = 0; Index < NewAttrs.Num(); ++Index)
                    {
                        const auto& Entry = NewAttrs[Index];

                        auto AttributeEntity = UCk_Utils_VectorAttribute_UE::TryGet(Entity, Entry.Get_AttributeName());
                        if (ck::Is_NOT_Valid(AttributeEntity))
                        {
                            Result = ECk_Persistence_ApplyResult::NotReady;
                            continue;
                        }

                        const auto UnchangedSinceLastApply = OldAttrs != nullptr &&
                            OldAttrs->IsValidIndex(Index) && (*OldAttrs)[Index] == Entry;
                        if (UnchangedSinceLastApply)
                        { continue; }

                        const auto CurrentValues = FCk_Fragment_VectorAttribute_BaseFinal
                        {
                            Entry.Get_AttributeName(),
                            UCk_Utils_VectorAttribute_UE::Get_BaseValue(AttributeEntity, Entry.Get_Component()),
                            UCk_Utils_VectorAttribute_UE::Get_FinalValue(AttributeEntity, Entry.Get_Component()),
                            Entry.Get_Component()
                        };
                        if (CurrentValues == Entry)
                        { continue; }

                        ck::attribute::Verbose(TEXT("Replicating VECTOR Attribute [{}] to [{}|{}]"),
                            Entry.Get_AttributeName(), Entry.Get_Base(), Entry.Get_Final());

                        ApplyReplicatedVectorAttributeEntry(AttributeEntity, Entry);
                    }

                    return Result;
                },
                .HydrationApply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_Persistence_ApplyResult
                {
                    return ck::attribute_restore::HydrationApply<ck::TFragment_VectorAttribute, FCk_RepData_VectorAttributes, UCk_Utils_VectorAttribute_UE>(
                        Entity, New, &ApplyReplicatedVectorAttributeEntry);
                }});
    }
} GVectorAttributeRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------