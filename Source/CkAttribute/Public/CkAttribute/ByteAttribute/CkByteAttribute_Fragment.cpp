#include "CkByteAttribute_Fragment.h"

#include "CkAttribute/CkAttribute_Log.h"
#include "CkAttribute/ByteAttribute/CkByteAttribute_Utils.h"

#include "CkCore/Math/Arithmetic/CkArithmetic_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"
#include "CkAttribute/CkAttribute_RestorePersistence.h" // RegisterLazyTyped<T> + shared attribute Produce/SeedContainer

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
    UCk_Utils_ByteAttributeModifier_UE::Request_ClearAllModifiers(InAttributeEntity, InEntry.Get_Component());
    UCk_Utils_ByteAttribute_UE::Request_Override(InAttributeEntity, InEntry.Get_Base(), InEntry.Get_Component());

    // The Final modifier's Add/Subtract operation tag is frozen at creation and Override's delta
    // parameter is unsigned (uint8), so a re-apply whose Final−Base difference changed sign
    // cannot be expressed by mutating the existing modifier in place — the old else-branch
    // wrapped the negative int difference through uint8 AND applied it with the stale operation,
    // producing garbage client values. Recreate through the same sign-aware path as the first
    // apply instead. (Float/Integer/Vector/Rotator keep their in-place Override: their deltas
    // are signed, always applied with the Add operation.)
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
// Container-based replication handler for Byte Attributes

static struct FByteAttributeRepHandlerRegistrar
{
    FByteAttributeRepHandlerRegistrar()
    {
        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazyTyped<FCk_RepData_ByteAttributes>(
            {
                .Apply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& Old) -> ECk_RepFragment_ApplyResult
                {
                    const auto& NewAttrs = New.Get<FCk_RepData_ByteAttributes>().Attributes;
                    const auto* OldAttrs = Old.IsSet()
                        ? &Old.GetValue().Get<FCk_RepData_ByteAttributes>().Attributes
                        : nullptr;

                    auto Result = ECk_RepFragment_ApplyResult::Applied;

                    for (auto Index = 0; Index < NewAttrs.Num(); ++Index)
                    {
                        const auto& Entry = NewAttrs[Index];

                        auto AttributeEntity = UCk_Utils_ByteAttribute_UE::TryGet(Entity, Entry.Get_AttributeName());
                        if (ck::Is_NOT_Valid(AttributeEntity))
                        {
                            // Not composed yet — keep the whole container entry pending. Siblings
                            // that did apply are skipped on the retry by the value check below.
                            Result = ECk_RepFragment_ApplyResult::NotReady;
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
                // Save-load hydration (authority-side, Phase 4B): the v3 payload is CHILD-keyed (per-attribute-entity
                // Produce), so Entity IS the attribute entity — write its value directly via ApplyReplicatedByteAttributeEntry.
                // The OWNER-keyed net Apply above never resolves it.
                .HydrationApply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_RepFragment_ApplyResult
                {
                    return ck::attribute_restore::HydrationApply<ck::TFragment_ByteAttribute, FCk_RepData_ByteAttributes>(
                        Entity, New, &ApplyReplicatedByteAttributeEntry);
                },
                .Produce       = &ck::attribute_restore::Produce<ck::TFragment_ByteAttribute, FCk_RepData_ByteAttributes>,
            });
    }
} GByteAttributeRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
