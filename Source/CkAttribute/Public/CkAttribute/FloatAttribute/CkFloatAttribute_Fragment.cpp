#include "CkFloatAttribute_Fragment.h"

#include "CkAttribute/CkAttribute_Log.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Utils.h"

#include "CkCore/Math/Arithmetic/CkArithmetic_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"
#include "CkAttribute/CkAttribute_RestorePersistence.h" // shared attribute Produce + HydrationApply helpers
#include "CkAttribute/CkAttribute_RefillPersistence.h"  // shared refill run-state Produce + HydrationApply helpers

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Fragment_FloatAttribute_BaseFinal::
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

// --------------------------------------------------------------------------------------------------------------------
// Container-based replication handler for Float Attributes

static struct FFloatAttributeRepHandlerRegistrar
{
    FFloatAttributeRepHandlerRegistrar()
    {
        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazyTyped<FCk_RepData_FloatAttributes>(
            {
                .Apply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& Old) -> ECk_RepFragment_ApplyResult
                {
                    const auto& NewAttrs = New.Get<FCk_RepData_FloatAttributes>().Attributes;
                    const auto* OldAttrs = Old.IsSet()
                        ? &Old.GetValue().Get<FCk_RepData_FloatAttributes>().Attributes
                        : nullptr;

                    auto Result = ECk_RepFragment_ApplyResult::Applied;

                    for (auto Index = 0; Index < NewAttrs.Num(); ++Index)
                    {
                        const auto& Entry = NewAttrs[Index];

                        auto AttributeEntity = UCk_Utils_FloatAttribute_UE::TryGet(Entity, Entry.Get_AttributeName());
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

                        const auto CurrentValues = FCk_Fragment_FloatAttribute_BaseFinal
                        {
                            Entry.Get_AttributeName(),
                            UCk_Utils_FloatAttribute_UE::Get_BaseValue(AttributeEntity, Entry.Get_Component()),
                            UCk_Utils_FloatAttribute_UE::Get_FinalValue(AttributeEntity, Entry.Get_Component()),
                            Entry.Get_Component()
                        };
                        if (CurrentValues == Entry)
                        { continue; }

                        ck::attribute::Verbose(TEXT("Replicating FLOAT Attribute [{}] to [{}|{}]"),
                            Entry.Get_AttributeName(), Entry.Get_Base(), Entry.Get_Final());

                        ApplyReplicatedFloatAttributeEntry(AttributeEntity, Entry);
                    }

                    return Result;
                },
                // Save-load hydration (authority-side, Phase 4B): the v3 payload is CHILD-keyed (per-attribute-entity
                // Produce), so Entity IS the attribute entity — write its value directly via ApplyReplicatedFloatAttributeEntry.
                // The OWNER-keyed net Apply above never resolves it.
                .HydrationApply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_RepFragment_ApplyResult
                {
                    return ck::attribute_restore::HydrationApply<ck::TFragment_FloatAttribute, FCk_RepData_FloatAttributes>(
                        Entity, New, &ApplyReplicatedFloatAttributeEntry);
                },
                .Produce       = &ck::attribute_restore::Produce<ck::TFragment_FloatAttribute, FCk_RepData_FloatAttributes>,
            });
    }
} GFloatAttributeRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
// Save-only handler for the Float attribute REFILL run-state (Running/Paused). Distinct from the VALUE handler above:
// the run-state is never on the wire, so this handler has NO net Apply — Produce + HydrationApply only. Both fire on
// the refill CHILD entity (per-attribute-entity keying). See CkAttribute_RefillPersistence.h.

static struct FFloatAttributeRefillRepHandlerRegistrar
{
    FFloatAttributeRefillRepHandlerRegistrar()
    {
        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazyTyped<FCk_SaveData_FloatAttributeRefill>(
            {
                .HydrationApply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_RepFragment_ApplyResult
                {
                    return ck::attribute_refill_restore::HydrationApply<
                        ck::TFragment_FloatAttribute, FCk_Handle_FloatAttributeRefill, UCk_Utils_FloatAttributeRefill_UE, FCk_SaveData_FloatAttributeRefill>(
                            Entity, New);
                },
                .Produce = &ck::attribute_refill_restore::Produce<
                    ck::TFragment_FloatAttribute, FCk_Handle_FloatAttributeRefill, UCk_Utils_FloatAttributeRefill_UE, FCk_SaveData_FloatAttributeRefill>,
            });
    }
} GFloatAttributeRefillRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------