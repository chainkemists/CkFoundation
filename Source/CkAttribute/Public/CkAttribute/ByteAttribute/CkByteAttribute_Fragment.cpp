#include "CkByteAttribute_Fragment.h"

#include "CkAttribute/CkAttribute_Log.h"
#include "CkAttribute/ByteAttribute/CkByteAttribute_Utils.h"

#include "CkCore/Math/Arithmetic/CkArithmetic_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Fragment_ByteAttribute_BaseFinal::
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
// Container-based replication handler for Byte Attributes

static struct FByteAttributeRepHandlerRegistrar
{
    FByteAttributeRepHandlerRegistrar()
    {
        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
            []() -> UScriptStruct* { return FCk_RepData_ByteAttributes::StaticStruct(); },
            {
                .OnChange = [](FCk_Handle& Entity, const FInstancedStruct& New, const FInstancedStruct& Old)
                {
                    const auto& NewAttrs = New.Get<FCk_RepData_ByteAttributes>().Attributes;
                    const auto& OldAttrs = Old.Get<FCk_RepData_ByteAttributes>().Attributes;

                    for (auto Index = 0; Index < NewAttrs.Num(); ++Index)
                    {
                        const auto& Entry = NewAttrs[Index];

                        auto AttributeEntity = UCk_Utils_ByteAttribute_UE::TryGet(Entity, Entry.Get_AttributeName());
                        if (ck::Is_NOT_Valid(AttributeEntity))
                        { continue; }

                        if (!OldAttrs.IsValidIndex(Index))
                        {
                            ck::attribute::Verbose(TEXT("Replicating BYTE Attribute [{}] for the FIRST time to [{}|{}]"),
                                Entry.Get_AttributeName(), Entry.Get_Base(), Entry.Get_Final());

                            UCk_Utils_ByteAttributeModifier_UE::Request_ClearAllModifiers(AttributeEntity, Entry.Get_Component());
                            UCk_Utils_ByteAttribute_UE::Request_Override(AttributeEntity, Entry.Get_Base(), Entry.Get_Component());

                            const auto& MaybeModifier = UCk_Utils_ByteAttributeModifier_UE::TryGet(AttributeEntity,
                                ck::FAttributeModifier_ReplicationTags::Get_FinalTag(), Entry.Get_Component());

                            CK_ENSURE_IF_NOT(ck::Is_NOT_Valid(MaybeModifier),
                                TEXT("Did not expect a Final Modifier [{}] to already exist on BYTE Attribute [{}]"),
                                ck::FAttributeModifier_ReplicationTags::Get_FinalTag(), AttributeEntity)
                            { continue; }

                            const auto Difference = Entry.Get_Final() - Entry.Get_Base();

                            UCk_Utils_ByteAttributeModifier_UE::Add_Revocable
                            (
                                AttributeEntity,
                                ck::FAttributeModifier_ReplicationTags::Get_FinalTag(),
                                Difference >= 0 ? ECk_AttributeModifier_Operation::Add : ECk_AttributeModifier_Operation::Subtract,
                                FCk_Fragment_ByteAttributeModifier_ParamsData
                                {
                                    static_cast<uint8>(std::abs(Difference)),
                                    Entry.Get_Component()
                                }
                            );

                            continue;
                        }

                        if (OldAttrs[Index] != Entry)
                        {
                            ck::attribute::Verbose(TEXT("Replicating BYTE Attribute [{}] and UPDATING it to [{}|{}]"),
                                Entry.Get_AttributeName(), Entry.Get_Base(), Entry.Get_Final());

                            UCk_Utils_ByteAttributeModifier_UE::Request_ClearAllModifiers(AttributeEntity, Entry.Get_Component());
                            UCk_Utils_ByteAttribute_UE::Request_Override(AttributeEntity, Entry.Get_Base(), Entry.Get_Component());

                            auto AttributeModifier = UCk_Utils_ByteAttributeModifier_UE::TryGet(AttributeEntity,
                                ck::FAttributeModifier_ReplicationTags::Get_FinalTag(), Entry.Get_Component());

                            if (ck::Is_NOT_Valid(AttributeModifier))
                            {
                                const auto Difference = Entry.Get_Final() - Entry.Get_Base();

                                UCk_Utils_ByteAttributeModifier_UE::Add_Revocable
                                (
                                    AttributeEntity,
                                    ck::FAttributeModifier_ReplicationTags::Get_FinalTag(),
                                    Difference >= 0 ? ECk_AttributeModifier_Operation::Add : ECk_AttributeModifier_Operation::Subtract,
                                    FCk_Fragment_ByteAttributeModifier_ParamsData
                                    {
                                        static_cast<uint8>(std::abs(Difference)),
                                        Entry.Get_Component()
                                    }
                                );
                            }
                            else
                            {
                                UCk_Utils_ByteAttributeModifier_UE::Override(
                                    AttributeModifier, Entry.Get_Final() - Entry.Get_Base());
                            }

                            continue;
                        }
                    }
                },
                .OnAdd = [](FCk_Handle& Entity, const FInstancedStruct& Data)
                {
                    const auto& Attributes = Data.Get<FCk_RepData_ByteAttributes>().Attributes;

                    for (const auto& Entry : Attributes)
                    {
                        auto AttributeEntity = UCk_Utils_ByteAttribute_UE::TryGet(Entity, Entry.Get_AttributeName());
                        if (ck::Is_NOT_Valid(AttributeEntity))
                        { continue; }

                        ck::attribute::Verbose(TEXT("Replicating BYTE Attribute [{}] for the FIRST time to [{}|{}]"),
                            Entry.Get_AttributeName(), Entry.Get_Base(), Entry.Get_Final());

                        UCk_Utils_ByteAttributeModifier_UE::Request_ClearAllModifiers(AttributeEntity, Entry.Get_Component());
                        UCk_Utils_ByteAttribute_UE::Request_Override(AttributeEntity, Entry.Get_Base(), Entry.Get_Component());

                        const auto Difference = Entry.Get_Final() - Entry.Get_Base();

                        UCk_Utils_ByteAttributeModifier_UE::Add_Revocable
                        (
                            AttributeEntity,
                            ck::FAttributeModifier_ReplicationTags::Get_FinalTag(),
                            Difference >= 0 ? ECk_AttributeModifier_Operation::Add : ECk_AttributeModifier_Operation::Subtract,
                            FCk_Fragment_ByteAttributeModifier_ParamsData
                            {
                                static_cast<uint8>(std::abs(Difference)),
                                Entry.Get_Component()
                            }
                        );
                    }
                }
            });
    }
} GByteAttributeRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------
