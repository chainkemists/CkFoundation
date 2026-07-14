#include "CkFloatAttribute_Fragment.h"

#include "CkAttribute/CkAttribute_Log.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Utils.h"

#include "CkCore/Math/Arithmetic/CkArithmetic_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"
#include "CkAttribute/CkAttribute_RestorePersistence.h" // RegisterLazyTyped<T> + shared attribute Produce/SeedContainer

#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Writer.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Reader.h"

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

using FSnap_RecordOfFloatAttributes = ck::FFragment_RecordOfFloatAttributes;
CK_REGISTER_SNAPSHOTABLE(FSnap_RecordOfFloatAttributes);

// Tier-C TFragment_EntityHolder specializations for the float refill relationship.
using FSnap_RefillAttribute_Float       = ck::TFragment_RefillAttribute<FCk_Handle_FloatAttributeRefill>;
using FSnap_RefillAttributeTarget_Float = ck::TFragment_RefillAttributeTarget<FCk_Handle_FloatAttribute>;

CK_REGISTER_SNAPSHOTABLE(FSnap_RefillAttribute_Float);
CK_REGISTER_SNAPSHOTABLE(FSnap_RefillAttributeTarget_Float);

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
                    // Save-load hydration (authority-side, Phase 4B): the v3 payload is CHILD-keyed (per-attribute-entity
                    // Produce), so under hydration Entity IS the attribute entity — write its value directly via
                    // ApplyReplicatedFloatAttributeEntry. The OWNER-keyed loop below never resolves it. Unset => not a
                    // hydration apply => fall through (net receive path byte-identical, gated on FCk_HydrationApplyScope).
                    if (const auto Hydrated = ck::attribute_restore::TryHydrationApply<ck::TFragment_FloatAttribute, FCk_RepData_FloatAttributes>(
                            Entity, New, &ApplyReplicatedFloatAttributeEntry);
                        Hydrated.IsSet())
                    { return *Hydrated; }

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
                .Produce       = &ck::attribute_restore::Produce<ck::TFragment_FloatAttribute, FCk_RepData_FloatAttributes>,
                .SeedContainer = &ck::attribute_restore::SeedContainer<ck::TFragment_FloatAttribute, FCk_RepData_FloatAttributes>,
                .Transport     = ECk_PersistenceTransport::NetAndSave // v3 save capture (Phase 3A.4)
            });
    }
} GFloatAttributeRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------