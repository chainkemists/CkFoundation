#include "CkByteAttribute_Fragment.h"

#include "CkAttribute/CkAttribute_Log.h"
#include "CkAttribute/ByteAttribute/CkByteAttribute_Utils.h"

#include "CkCore/Math/Arithmetic/CkArithmetic_Utils.h"

#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

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

auto
    UCk_Fragment_ByteAttribute_Rep::
    Broadcast_AddOrUpdate(
        FGameplayTag InAttributeName,
        uint8 InBase,
        uint8 InFinal,
        ECk_MinMaxCurrent InComponent)
    -> void
{
    const auto Found = _AttributesToReplicate.FindByPredicate([&](const FCk_Fragment_ByteAttribute_BaseFinal& InElement)
    {
        return InElement.Get_AttributeName() == InAttributeName && InElement.Get_Component() == InComponent;
    });

    const auto& ToReplicate = FCk_Fragment_ByteAttribute_BaseFinal{InAttributeName, InBase, InFinal, InComponent};

    if (ck::Is_NOT_Valid(Found, ck::IsValid_Policy_NullptrOnly{}))
    {
        _AttributesToReplicate.Emplace(ToReplicate);
    }
    else
    {
        *Found = ToReplicate;
    }

    MARK_PROPERTY_DIRTY_FROM_NAME(UCk_Fragment_ByteAttribute_Rep, _AttributesToReplicate, this);
}

auto
    UCk_Fragment_ByteAttribute_Rep::
    PostLink()
    -> void
{
    OnRep_Updated();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Fragment_ByteAttribute_Rep::
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
    -> void
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    constexpr auto Params = FDoRepLifetimeParams{COND_None, REPNOTIFY_Always, true};

    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _AttributesToReplicate, Params);
}

auto
    UCk_Fragment_ByteAttribute_Rep::
    Request_TryUpdateReplicatedAttributes()
    -> void
{
    OnRep_Updated();
}

auto
    UCk_Fragment_ByteAttribute_Rep::
    OnRep_Updated()
    -> void
{
    if (ck::Is_NOT_Valid(Get_AssociatedEntity()))
    { return; }

    if (GetWorld()->IsNetMode(NM_DedicatedServer))
    { return; }

    for (auto Index = _AttributesToReplicate_Previous.Num(); Index < _AttributesToReplicate.Num(); ++Index)
    {
        const auto& AttributeToReplicate = _AttributesToReplicate[Index];

        if (const auto& AttributeEntity = UCk_Utils_ByteAttribute_UE::TryGet(Get_AssociatedEntity(), AttributeToReplicate.Get_AttributeName());
            ck::Is_NOT_Valid(AttributeEntity))
        {
            ck::attribute::Verbose(TEXT("Could NOT find BYTE Attribute [{}]. BYTE Attribute replication PENDING..."),
                AttributeToReplicate.Get_AttributeName());

            return;
        }
    }

    for (auto Index = 0; Index < _AttributesToReplicate.Num(); ++Index)
    {
        const auto& AttributeToReplicate = _AttributesToReplicate[Index];
        auto AttributeEntity = UCk_Utils_ByteAttribute_UE::TryGet(Get_AssociatedEntity(), AttributeToReplicate.Get_AttributeName());

        if (NOT _AttributesToReplicate_Previous.IsValidIndex(Index))
        {
            ck::attribute::Verbose(TEXT("Replicating BYTE Attribute [{}] for the FIRST time to [{}|{}]"), AttributeToReplicate.Get_AttributeName(),
                AttributeToReplicate.Get_Base(), AttributeToReplicate.Get_Final());

            UCk_Utils_ByteAttribute_UE::Request_Override(AttributeEntity, AttributeToReplicate.Get_Base(), AttributeToReplicate.Get_Component());

            const auto& MaybeModifier = UCk_Utils_ByteAttributeModifier_UE::TryGet(AttributeEntity, ck::FAttributeModifier_ReplicationTags::Get_FinalTag(),
                AttributeToReplicate.Get_Component());

            CK_ENSURE_IF_NOT(ck::Is_NOT_Valid(MaybeModifier), TEXT("Did not expect a Final Modifier [{}] to already exist on BYTE Attribute [{}]"),
                ck::FAttributeModifier_ReplicationTags::Get_FinalTag(), AttributeEntity)
            { continue; }

            const auto Difference = AttributeToReplicate.Get_Final() - AttributeToReplicate.Get_Base();

            UCk_Utils_ByteAttributeModifier_UE::Add_Revocable
            (
                AttributeEntity,
                ck::FAttributeModifier_ReplicationTags::Get_FinalTag(),
                Difference >= 0 ? ECk_AttributeModifier_Operation::Add : ECk_AttributeModifier_Operation::Subtract,
                FCk_Fragment_ByteAttributeModifier_ParamsData
                {
                    static_cast<uint8>(std::abs(Difference)),
                    AttributeToReplicate.Get_Component()
                }
            );

            continue;
        }

        if (_AttributesToReplicate_Previous[Index] != AttributeToReplicate)
        {
            ck::attribute::Verbose(TEXT("Replicating BYTE Attribute [{}] and UPDATING it to [{}|{}]"), AttributeToReplicate.Get_AttributeName(),
                AttributeToReplicate.Get_Base(), AttributeToReplicate.Get_Final());

            UCk_Utils_ByteAttribute_UE::Request_Override(
                AttributeEntity, AttributeToReplicate.Get_Base(), AttributeToReplicate.Get_Component());

            if (auto AttributeModifier = UCk_Utils_ByteAttributeModifier_UE::TryGet(AttributeEntity,
                ck::FAttributeModifier_ReplicationTags::Get_FinalTag(), AttributeToReplicate.Get_Component());
                ck::IsValid(AttributeModifier))
            {
                UCk_Utils_ByteAttributeModifier_UE::Override(
                    AttributeModifier, AttributeToReplicate.Get_Final() - AttributeToReplicate.Get_Base());
            }
            else
            {
                const auto Difference = AttributeToReplicate.Get_Final() - AttributeToReplicate.Get_Base();

                UCk_Utils_ByteAttributeModifier_UE::Add_Revocable
                (
                    AttributeEntity,
                    ck::FAttributeModifier_ReplicationTags::Get_FinalTag(),
                    Difference >= 0 ? ECk_AttributeModifier_Operation::Add : ECk_AttributeModifier_Operation::Subtract,
                    FCk_Fragment_ByteAttributeModifier_ParamsData
                    {
                        static_cast<uint8>(std::abs(Difference)),
                        AttributeToReplicate.Get_Component()
                    }
                );
            }

            continue;
        }

        ck::attribute::Verbose(TEXT("IGNORING BYTE Attribute [{}] as there is no change between [{}] and [{}]"),
            AttributeToReplicate.Get_AttributeName());
    }

    _AttributesToReplicate_Previous = _AttributesToReplicate;
}

// --------------------------------------------------------------------------------------------------------------------
