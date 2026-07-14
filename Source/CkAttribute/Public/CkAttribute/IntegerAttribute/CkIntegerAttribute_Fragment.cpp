#include "CkIntegerAttribute_Fragment.h"

#include "CkAttribute/CkAttribute_Log.h"
#include "CkAttribute/IntegerAttribute/CkIntegerAttribute_Utils.h"

#include "CkCore/Math/Arithmetic/CkArithmetic_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"
#include "CkAttribute/CkAttribute_RestorePersistence.h" // RegisterLazyTyped<T> + shared attribute Produce/SeedContainer

#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Writer.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Reader.h"

// --------------------------------------------------------------------------------------------------------------------
// Tier-C SerializeSnapshot registration for the Integer attribute family. ck:: types are hoisted to unqualified
// file-scope aliases because CK_REGISTER_SNAPSHOTABLE token-pastes the type name (the `::` cannot be pasted).

using FSnap_IntegerAttribute_Current = ck::FFragment_IntegerAttribute_Current;
using FSnap_IntegerAttribute_Min     = ck::FFragment_IntegerAttribute_Min;
using FSnap_IntegerAttribute_Max     = ck::FFragment_IntegerAttribute_Max;

CK_REGISTER_SNAPSHOTABLE(FSnap_IntegerAttribute_Current);
CK_REGISTER_SNAPSHOTABLE(FSnap_IntegerAttribute_Min);
CK_REGISTER_SNAPSHOTABLE(FSnap_IntegerAttribute_Max);

using FSnap_IntegerAttributeModifier_Current = ck::FFragment_IntegerAttributeModifier_Current;
using FSnap_IntegerAttributeModifier_Min     = ck::FFragment_IntegerAttributeModifier_Min;
using FSnap_IntegerAttributeModifier_Max     = ck::FFragment_IntegerAttributeModifier_Max;

CK_REGISTER_SNAPSHOTABLE(FSnap_IntegerAttributeModifier_Current);
CK_REGISTER_SNAPSHOTABLE(FSnap_IntegerAttributeModifier_Min);
CK_REGISTER_SNAPSHOTABLE(FSnap_IntegerAttributeModifier_Max);

using FSnap_IntegerAttribute_PreviousValues_Current = ck::TFragment_Attribute_PreviousValues<ck::FFragment_IntegerAttribute_Current>;
using FSnap_IntegerAttribute_PreviousValues_Min     = ck::TFragment_Attribute_PreviousValues<ck::FFragment_IntegerAttribute_Min>;
using FSnap_IntegerAttribute_PreviousValues_Max     = ck::TFragment_Attribute_PreviousValues<ck::FFragment_IntegerAttribute_Max>;

CK_REGISTER_SNAPSHOTABLE(FSnap_IntegerAttribute_PreviousValues_Current);
CK_REGISTER_SNAPSHOTABLE(FSnap_IntegerAttribute_PreviousValues_Min);
CK_REGISTER_SNAPSHOTABLE(FSnap_IntegerAttribute_PreviousValues_Max);

using FSnap_RecordOfIntegerAttributes = ck::FFragment_RecordOfIntegerAttributes;
CK_REGISTER_SNAPSHOTABLE(FSnap_RecordOfIntegerAttributes);

// Tier-C TFragment_EntityHolder specializations for the integer refill relationship.
using FSnap_RefillAttribute_Integer       = ck::TFragment_RefillAttribute<FCk_Handle_IntegerAttributeRefill>;
using FSnap_RefillAttributeTarget_Integer = ck::TFragment_RefillAttributeTarget<FCk_Handle_IntegerAttribute>;

CK_REGISTER_SNAPSHOTABLE(FSnap_RefillAttribute_Integer);
CK_REGISTER_SNAPSHOTABLE(FSnap_RefillAttributeTarget_Integer);

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
// Container-based replication handler for Integer Attributes

static struct FIntegerAttributeRepHandlerRegistrar
{
    FIntegerAttributeRepHandlerRegistrar()
    {
        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazyTyped<FCk_RepData_IntegerAttributes>(
            {
                .Apply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& Old) -> ECk_RepFragment_ApplyResult
                {
                    // Save-load hydration (authority-side, Phase 4B): the v3 payload is CHILD-keyed (per-attribute-entity
                    // Produce), so under hydration Entity IS the attribute entity — write its value directly via
                    // ApplyReplicatedIntegerAttributeEntry. The OWNER-keyed loop below never resolves it. Unset => not a
                    // hydration apply => fall through (net receive path byte-identical, gated on FCk_HydrationApplyScope).
                    if (const auto Hydrated = ck::attribute_restore::TryHydrationApply<ck::TFragment_IntegerAttribute, FCk_RepData_IntegerAttributes>(
                            Entity, New, &ApplyReplicatedIntegerAttributeEntry);
                        Hydrated.IsSet())
                    { return *Hydrated; }

                    const auto& NewAttrs = New.Get<FCk_RepData_IntegerAttributes>().Attributes;
                    const auto* OldAttrs = Old.IsSet()
                        ? &Old.GetValue().Get<FCk_RepData_IntegerAttributes>().Attributes
                        : nullptr;

                    auto Result = ECk_RepFragment_ApplyResult::Applied;

                    for (auto Index = 0; Index < NewAttrs.Num(); ++Index)
                    {
                        const auto& Entry = NewAttrs[Index];

                        auto AttributeEntity = UCk_Utils_IntegerAttribute_UE::TryGet(Entity, Entry.Get_AttributeName());
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
                .Produce       = &ck::attribute_restore::Produce<ck::TFragment_IntegerAttribute, FCk_RepData_IntegerAttributes>,
                .Transport     = ECk_PersistenceTransport::NetAndSave // v3 save capture (Phase 3A.4)
            });
    }
} GIntegerAttributeRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------