#include "CkIntegerAttribute_Fragment.h"

#include "CkAttribute/CkAttribute_Log.h"
#include "CkAttribute/IntegerAttribute/CkIntegerAttribute_Utils.h"

#include "CkCore/Math/Arithmetic/CkArithmetic_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Fragment_IntegerAttribute_BaseFinal::
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
// Container-based replication handler for Integer Attributes

static struct FIntegerAttributeRepHandlerRegistrar
{
    FIntegerAttributeRepHandlerRegistrar()
    {
        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
            []() -> UScriptStruct* { return FCk_RepData_IntegerAttributes::StaticStruct(); },
            {
                .OnChange = [](FCk_Handle& Entity, const FInstancedStruct& New, const FInstancedStruct& Old)
                {
                    const auto& NewAttrs = New.Get<FCk_RepData_IntegerAttributes>().Attributes;
                    const auto& OldAttrs = Old.Get<FCk_RepData_IntegerAttributes>().Attributes;

                    for (auto Index = 0; Index < NewAttrs.Num(); ++Index)
                    {
                        const auto& Entry = NewAttrs[Index];

                        auto AttributeEntity = UCk_Utils_IntegerAttribute_UE::TryGet(Entity, Entry.Get_AttributeName());
                        if (ck::Is_NOT_Valid(AttributeEntity))
                        { continue; }

                        if (!OldAttrs.IsValidIndex(Index))
                        {
                            ck::attribute::Verbose(TEXT("Replicating INTEGER Attribute [{}] for the FIRST time to [{}|{}]"),
                                Entry.Get_AttributeName(), Entry.Get_Base(), Entry.Get_Final());

                            UCk_Utils_IntegerAttributeModifier_UE::Request_ClearAllModifiers(AttributeEntity, Entry.Get_Component());
                            UCk_Utils_IntegerAttribute_UE::Request_Override(AttributeEntity, Entry.Get_Base(), Entry.Get_Component());

                            const auto& MaybeModifier = UCk_Utils_IntegerAttributeModifier_UE::TryGet(AttributeEntity,
                                ck::FAttributeModifier_ReplicationTags::Get_FinalTag(), Entry.Get_Component());

                            CK_ENSURE_IF_NOT(ck::Is_NOT_Valid(MaybeModifier),
                                TEXT("Did not expect a Final Modifier [{}] to already exist on INTEGER Attribute [{}]"),
                                ck::FAttributeModifier_ReplicationTags::Get_FinalTag(), AttributeEntity)
                            { continue; }

                            UCk_Utils_IntegerAttributeModifier_UE::Add_Revocable
                            (
                                AttributeEntity,
                                ck::FAttributeModifier_ReplicationTags::Get_FinalTag(),
                                ECk_AttributeModifier_Operation::Add,
                                FCk_Fragment_IntegerAttributeModifier_ParamsData
                                {
                                    Entry.Get_Final() - Entry.Get_Base(),
                                    Entry.Get_Component()
                                }
                            );

                            continue;
                        }

                        if (OldAttrs[Index] != Entry)
                        {
                            ck::attribute::Verbose(TEXT("Replicating INTEGER Attribute [{}] and UPDATING it to [{}|{}]"),
                                Entry.Get_AttributeName(), Entry.Get_Base(), Entry.Get_Final());

                            UCk_Utils_IntegerAttributeModifier_UE::Request_ClearAllModifiers(AttributeEntity, Entry.Get_Component());
                            UCk_Utils_IntegerAttribute_UE::Request_Override(AttributeEntity, Entry.Get_Base(), Entry.Get_Component());

                            auto AttributeModifier = UCk_Utils_IntegerAttributeModifier_UE::TryGet(AttributeEntity,
                                ck::FAttributeModifier_ReplicationTags::Get_FinalTag(), Entry.Get_Component());

                            CK_ENSURE_IF_NOT(ck::IsValid(AttributeModifier),
                                TEXT("Did not expect the Final Modifier [{}] to NOT exist on INTEGER Attribute [{}]"),
                                ck::FAttributeModifier_ReplicationTags::Get_FinalTag(), AttributeEntity)
                            { continue; }

                            UCk_Utils_IntegerAttributeModifier_UE::Override(
                                AttributeModifier, Entry.Get_Final() - Entry.Get_Base());

                            continue;
                        }
                    }
                },
                .OnAdd = [](FCk_Handle& Entity, const FInstancedStruct& Data)
                {
                    const auto& Attributes = Data.Get<FCk_RepData_IntegerAttributes>().Attributes;

                    for (const auto& Entry : Attributes)
                    {
                        auto AttributeEntity = UCk_Utils_IntegerAttribute_UE::TryGet(Entity, Entry.Get_AttributeName());
                        if (ck::Is_NOT_Valid(AttributeEntity))
                        { continue; }

                        ck::attribute::Verbose(TEXT("Replicating INTEGER Attribute [{}] for the FIRST time to [{}|{}]"),
                            Entry.Get_AttributeName(), Entry.Get_Base(), Entry.Get_Final());

                        UCk_Utils_IntegerAttributeModifier_UE::Request_ClearAllModifiers(AttributeEntity, Entry.Get_Component());
                        UCk_Utils_IntegerAttribute_UE::Request_Override(AttributeEntity, Entry.Get_Base(), Entry.Get_Component());

                        UCk_Utils_IntegerAttributeModifier_UE::Add_Revocable
                        (
                            AttributeEntity,
                            ck::FAttributeModifier_ReplicationTags::Get_FinalTag(),
                            ECk_AttributeModifier_Operation::Add,
                            FCk_Fragment_IntegerAttributeModifier_ParamsData
                            {
                                Entry.Get_Final() - Entry.Get_Base(),
                                Entry.Get_Component()
                            }
                        );
                    }
                }
            });
    }
} GIntegerAttributeRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------