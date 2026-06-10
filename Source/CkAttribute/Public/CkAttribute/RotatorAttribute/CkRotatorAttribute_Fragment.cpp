#include "CkRotatorAttribute_Fragment.h"

#include "CkAttribute/CkAttribute_Log.h"
#include "CkAttribute/RotatorAttribute/CkRotatorAttribute_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Writer.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Reader.h"

// --------------------------------------------------------------------------------------------------------------------
// Tier-C SerializeSnapshot registration for the Rotator attribute family — mirrors the Float family.
// CK_REGISTER_SNAPSHOTABLE token-pastes the type name, so ck:: types are hoisted to file-scope aliases.

using FSnap_RotatorAttribute_Current = ck::FFragment_RotatorAttribute_Current;
using FSnap_RotatorAttribute_Min     = ck::FFragment_RotatorAttribute_Min;
using FSnap_RotatorAttribute_Max     = ck::FFragment_RotatorAttribute_Max;

CK_REGISTER_SNAPSHOTABLE(FSnap_RotatorAttribute_Current);
CK_REGISTER_SNAPSHOTABLE(FSnap_RotatorAttribute_Min);
CK_REGISTER_SNAPSHOTABLE(FSnap_RotatorAttribute_Max);

using FSnap_RotatorAttributeModifier_Current = ck::FFragment_RotatorAttributeModifier_Current;
using FSnap_RotatorAttributeModifier_Min     = ck::FFragment_RotatorAttributeModifier_Min;
using FSnap_RotatorAttributeModifier_Max     = ck::FFragment_RotatorAttributeModifier_Max;

CK_REGISTER_SNAPSHOTABLE(FSnap_RotatorAttributeModifier_Current);
CK_REGISTER_SNAPSHOTABLE(FSnap_RotatorAttributeModifier_Min);
CK_REGISTER_SNAPSHOTABLE(FSnap_RotatorAttributeModifier_Max);

using FSnap_RotatorAttribute_PreviousValues_Current = ck::TFragment_Attribute_PreviousValues<ck::FFragment_RotatorAttribute_Current>;
using FSnap_RotatorAttribute_PreviousValues_Min     = ck::TFragment_Attribute_PreviousValues<ck::FFragment_RotatorAttribute_Min>;
using FSnap_RotatorAttribute_PreviousValues_Max     = ck::TFragment_Attribute_PreviousValues<ck::FFragment_RotatorAttribute_Max>;

CK_REGISTER_SNAPSHOTABLE(FSnap_RotatorAttribute_PreviousValues_Current);
CK_REGISTER_SNAPSHOTABLE(FSnap_RotatorAttribute_PreviousValues_Min);
CK_REGISTER_SNAPSHOTABLE(FSnap_RotatorAttribute_PreviousValues_Max);

using FSnap_RecordOfRotatorAttributes = ck::FFragment_RecordOfRotatorAttributes;
CK_REGISTER_SNAPSHOTABLE(FSnap_RecordOfRotatorAttributes);

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Fragment_RotatorAttribute_BaseFinal::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return _AttributeName == InOther.Get_AttributeName() &&
        _Component == InOther.Get_Component() &&
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

// --------------------------------------------------------------------------------------------------------------------
// Container-based replication handler for Rotator Attributes

static struct FRotatorAttributeRepHandlerRegistrar
{
    FRotatorAttributeRepHandlerRegistrar()
    {
        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
            []() -> UScriptStruct* { return FCk_RepData_RotatorAttributes::StaticStruct(); },
            {
                .Apply = [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& Old) -> ECk_RepFragment_ApplyResult
                {
                    const auto& NewAttrs = New.Get<FCk_RepData_RotatorAttributes>().Attributes;
                    const auto* OldAttrs = Old.IsSet()
                        ? &Old.GetValue().Get<FCk_RepData_RotatorAttributes>().Attributes
                        : nullptr;

                    auto Result = ECk_RepFragment_ApplyResult::Applied;

                    for (auto Index = 0; Index < NewAttrs.Num(); ++Index)
                    {
                        const auto& Entry = NewAttrs[Index];

                        auto AttributeEntity = UCk_Utils_RotatorAttribute_UE::TryGet(Entity, Entry.Get_AttributeName());
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

                        const auto CurrentValues = FCk_Fragment_RotatorAttribute_BaseFinal
                        {
                            Entry.Get_AttributeName(),
                            UCk_Utils_RotatorAttribute_UE::Get_BaseValue(AttributeEntity, Entry.Get_Component()),
                            UCk_Utils_RotatorAttribute_UE::Get_FinalValue(AttributeEntity, Entry.Get_Component()),
                            Entry.Get_Component()
                        };
                        if (CurrentValues == Entry)
                        { continue; }

                        ck::attribute::Verbose(TEXT("Replicating ROTATOR Attribute [{}] to [{}|{}]"),
                            Entry.Get_AttributeName(), Entry.Get_Base(), Entry.Get_Final());

                        ApplyReplicatedRotatorAttributeEntry(AttributeEntity, Entry);
                    }

                    return Result;
                }
            });
    }
} GRotatorAttributeRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
