#include "CkByteAttribute_Fragment.h"

#include "CkAttribute/CkAttribute_Log.h"
#include "CkAttribute/ByteAttribute/CkByteAttribute_Utils.h"

#include "CkCore/Math/Arithmetic/CkArithmetic_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"
#include "CkAttribute/CkAttribute_RestorePersistence.h" // Register_* + shared attribute Produce/HydrationApply

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Fragment_ByteAttribute_BaseFinal::
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
    ApplyReplicatedByteAttributeEntry(
        FCk_Handle_ByteAttribute& InAttributeEntity,
        const FCk_Fragment_ByteAttribute_BaseFinal& InEntry)
    -> void
{
    UCk_Utils_ByteAttributeModifier_UE::Request_ClearAllModifiers(InAttributeEntity, InEntry.Get_Component(), {});
    UCk_Utils_ByteAttribute_UE::Request_Override(InAttributeEntity, InEntry.Get_Base(), InEntry.Get_Component(), {});

    // Byte-only: a sign-flipped re-apply must RECREATE the Final modifier, never Override it. See CkAttribute/CLAUDE.md.
    if (auto ExistingModifier = UCk_Utils_ByteAttributeModifier_UE::TryGet(InAttributeEntity,
            ck::FAttributeModifier_ReplicationTags::Get_FinalTag(), InEntry.Get_Component());
        ck::IsValid(ExistingModifier))
    {
        UCk_Utils_ByteAttributeModifier_UE::Remove(ExistingModifier);
    }

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

// --------------------------------------------------------------------------------------------------------------------

static struct FByteAttributeRepHandlerRegistrar
{
    FByteAttributeRepHandlerRegistrar()
    {
        FCk_PersistenceHandlerRegistry::Register_NetAndSave_SplitApply<FCk_RepData_ByteAttributes>({
                .Posture = ECk_Snapshot_Posture::Durable,
                .Produce = &ck::attribute_restore::Produce<ck::TFragment_ByteAttribute, FCk_RepData_ByteAttributes>,
                .NetApply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& Old) -> ECk_Persistence_ApplyResult
                {
                    const auto& NewAttrs = New.Get<FCk_RepData_ByteAttributes>().Attributes;
                    const auto* OldAttrs = Old.IsSet()
                        ? &Old.GetValue().Get<FCk_RepData_ByteAttributes>().Attributes
                        : nullptr;

                    auto Result = ECk_Persistence_ApplyResult::Applied;

                    for (auto Index = 0; Index < NewAttrs.Num(); ++Index)
                    {
                        const auto& Entry = NewAttrs[Index];

                        auto AttributeEntity = UCk_Utils_ByteAttribute_UE::TryGet(Entity, Entry.Get_AttributeName());
                        if (ck::Is_NOT_Valid(AttributeEntity))
                        {
                            Result = ECk_Persistence_ApplyResult::NotReady;
                            continue;
                        }

                        const auto UnchangedSinceLastApply = OldAttrs != nullptr &&
                            OldAttrs->IsValidIndex(Index) && (*OldAttrs)[Index] == Entry;
                        if (UnchangedSinceLastApply)
                        { continue; }

                        const auto CurrentValues = FCk_Fragment_ByteAttribute_BaseFinal
                        {
                            Entry.Get_AttributeName(),
                            UCk_Utils_ByteAttribute_UE::Get_BaseValue(AttributeEntity, Entry.Get_Component()),
                            UCk_Utils_ByteAttribute_UE::Get_FinalValue(AttributeEntity, Entry.Get_Component()),
                            Entry.Get_Component()
                        };
                        if (CurrentValues == Entry)
                        { continue; }

                        ck::attribute::Verbose(TEXT("Replicating BYTE Attribute [{}] to [{}|{}]"),
                            Entry.Get_AttributeName(), Entry.Get_Base(), Entry.Get_Final());

                        ApplyReplicatedByteAttributeEntry(AttributeEntity, Entry);
                    }

                    return Result;
                },
                .HydrationApply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_Persistence_ApplyResult
                {
                    return ck::attribute_restore::HydrationApply<ck::TFragment_ByteAttribute, FCk_RepData_ByteAttributes, UCk_Utils_ByteAttribute_UE>(
                        Entity, New, &ApplyReplicatedByteAttributeEntry);
                }});
    }
} GByteAttributeRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
