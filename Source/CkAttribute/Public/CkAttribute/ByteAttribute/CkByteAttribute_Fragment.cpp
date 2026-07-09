#include "CkByteAttribute_Fragment.h"

#include "CkAttribute/CkAttribute_Log.h"
#include "CkAttribute/ByteAttribute/CkByteAttribute_Utils.h"

#include "CkCore/Math/Arithmetic/CkArithmetic_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Writer.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Reader.h"

// --------------------------------------------------------------------------------------------------------------------
// Tier-C SerializeSnapshot registration for the Byte attribute family. ck:: types are hoisted to unqualified
// file-scope aliases because CK_REGISTER_SNAPSHOTABLE token-pastes the type name (the `::` cannot be pasted).

using FSnap_ByteAttribute_Current = ck::FFragment_ByteAttribute_Current;
using FSnap_ByteAttribute_Min     = ck::FFragment_ByteAttribute_Min;
using FSnap_ByteAttribute_Max     = ck::FFragment_ByteAttribute_Max;

CK_REGISTER_SNAPSHOTABLE(FSnap_ByteAttribute_Current);
CK_REGISTER_SNAPSHOTABLE(FSnap_ByteAttribute_Min);
CK_REGISTER_SNAPSHOTABLE(FSnap_ByteAttribute_Max);

using FSnap_ByteAttributeModifier_Current = ck::FFragment_ByteAttributeModifier_Current;
using FSnap_ByteAttributeModifier_Min     = ck::FFragment_ByteAttributeModifier_Min;
using FSnap_ByteAttributeModifier_Max     = ck::FFragment_ByteAttributeModifier_Max;

CK_REGISTER_SNAPSHOTABLE(FSnap_ByteAttributeModifier_Current);
CK_REGISTER_SNAPSHOTABLE(FSnap_ByteAttributeModifier_Min);
CK_REGISTER_SNAPSHOTABLE(FSnap_ByteAttributeModifier_Max);

using FSnap_ByteAttribute_PreviousValues_Current = ck::TFragment_Attribute_PreviousValues<ck::FFragment_ByteAttribute_Current>;
using FSnap_ByteAttribute_PreviousValues_Min     = ck::TFragment_Attribute_PreviousValues<ck::FFragment_ByteAttribute_Min>;
using FSnap_ByteAttribute_PreviousValues_Max     = ck::TFragment_Attribute_PreviousValues<ck::FFragment_ByteAttribute_Max>;

CK_REGISTER_SNAPSHOTABLE(FSnap_ByteAttribute_PreviousValues_Current);
CK_REGISTER_SNAPSHOTABLE(FSnap_ByteAttribute_PreviousValues_Min);
CK_REGISTER_SNAPSHOTABLE(FSnap_ByteAttribute_PreviousValues_Max);

using FSnap_RecordOfByteAttributes = ck::FFragment_RecordOfByteAttributes;
CK_REGISTER_SNAPSHOTABLE(FSnap_RecordOfByteAttributes);

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
        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
            []() -> UScriptStruct* { return FCk_RepData_ByteAttributes::StaticStruct(); },
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
                }
            });
    }
} GByteAttributeRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
