#include "CkFloatAttribute_Fragment.h"

#include "CkAttribute/CkAttribute_Log.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Utils.h"

#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Fragment_FloatAttribute_BaseFinal::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return _AttributeName == InOther.Get_AttributeName() &&
        _Component == Get_Component() &&
        FMath::IsNearlyEqual(_Base, InOther.Get_Base()) &&
        FMath::IsNearlyEqual(_Final, InOther.Get_Final());
}

auto
    UCk_Fragment_FloatAttribute_Rep::
    Broadcast_AddOrUpdate(
        FGameplayTag InAttributeName,
        float InBase,
        float InFinal,
        ECk_MinMaxCurrent InComponent)
    -> void
{
    const auto Found = _AttributesToReplicate.FindByPredicate([&](const FCk_Fragment_FloatAttribute_BaseFinal& InElement)
    {
        return InElement.Get_AttributeName() == InAttributeName;
    });

    const auto& ToReplicate = FCk_Fragment_FloatAttribute_BaseFinal{InAttributeName, InBase, InFinal, InComponent};

    if (ck::Is_NOT_Valid(Found, ck::IsValid_Policy_NullptrOnly{}))
    {
        _AttributesToReplicate.Emplace(ToReplicate);
    }
    else
    {
        *Found = ToReplicate;
    }

    MARK_PROPERTY_DIRTY_FROM_NAME(UCk_Fragment_FloatAttribute_Rep, _AttributesToReplicate, this);
}

auto
    UCk_Fragment_FloatAttribute_Rep::
    PostLink()
    -> void
{
    OnRep_Updated();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Fragment_FloatAttribute_Rep::
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
    -> void
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    constexpr auto Params = FDoRepLifetimeParams{COND_None, REPNOTIFY_Always, true};

    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _AttributesToReplicate, Params);
}

auto
    UCk_Fragment_FloatAttribute_Rep::
    Request_TryUpdateReplicatedAttributes()
    -> void
{
    OnRep_Updated();
}

auto
    UCk_Fragment_FloatAttribute_Rep::
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
        auto AttributeEntity = UCk_Utils_FloatAttribute_UE::TryGet(Get_AssociatedEntity(), AttributeToReplicate.Get_AttributeName());

        if (ck::Is_NOT_Valid(AttributeEntity))
        {
            ck::attribute::Verbose(TEXT("Could NOT find FLOAT Attribute [{}]. Float Attribute replication PENDING..."),
                AttributeToReplicate.Get_AttributeName());
            return;
        }
    }

    for (auto Index = 0; Index < _AttributesToReplicate.Num(); ++Index)
    {
        const auto& AttributeToReplicate = _AttributesToReplicate[Index];
        auto AttributeEntity = UCk_Utils_FloatAttribute_UE::TryGet(Get_AssociatedEntity(), AttributeToReplicate.Get_AttributeName());

        if (NOT _AttributesToReplicate_Previous.IsValidIndex(Index))
        {
            ck::attribute::Verbose(TEXT("Replicating FLOAT Attribute [{}] for the FIRST time to [{}|{}]"), AttributeToReplicate.Get_AttributeName(),
                AttributeToReplicate.Get_Base(), AttributeToReplicate.Get_Final());

            // Update the attribute
            UCk_Utils_FloatAttribute_UE::Request_Override(AttributeEntity, AttributeToReplicate.Get_Base(), AttributeToReplicate.Get_Component());

            const auto& MaybeModifier = UCk_Utils_FloatAttributeModifier_UE::TryGet(AttributeEntity, ck::FAttributeModifier_ReplicationTags::Get_FinalTag(),
                AttributeToReplicate.Get_Component());

            CK_ENSURE_IF_NOT(ck::Is_NOT_Valid(MaybeModifier), TEXT("Did not expect a Final Modifier [{}] to already exist on FLOAT Attribute [{}]"),
                ck::FAttributeModifier_ReplicationTags::Get_FinalTag(), AttributeEntity)
            { continue; }

            UCk_Utils_FloatAttributeModifier_UE::Add
            (
                AttributeEntity,
                ck::FAttributeModifier_ReplicationTags::Get_FinalTag(),
                FCk_Fragment_FloatAttributeModifier_ParamsData
                {
                    AttributeToReplicate.Get_Final() - AttributeToReplicate.Get_Base(),
                    ECk_ArithmeticOperations_Basic::Add,
                    ECk_ModifierOperation_RevocablePolicy::Revocable,
                    AttributeToReplicate.Get_Component()
                }
            );

            continue;
        }

        if (_AttributesToReplicate_Previous[Index] != AttributeToReplicate)
        {
            ck::attribute::Verbose(TEXT("Replicating FLOAT Attribute [{}] and UPDATING it to [{}|{}]"), AttributeToReplicate.Get_AttributeName(),
                AttributeToReplicate.Get_Base(), AttributeToReplicate.Get_Final());

            UCk_Utils_FloatAttribute_UE::Request_Override(
                AttributeEntity, AttributeToReplicate.Get_Base(), AttributeToReplicate.Get_Component());

            auto AttributeModifier = UCk_Utils_FloatAttributeModifier_UE::TryGet(AttributeEntity,
                ck::FAttributeModifier_ReplicationTags::Get_FinalTag(), AttributeToReplicate.Get_Component());

            if (ck::Is_NOT_Valid(AttributeModifier))
            {
                UCk_Utils_FloatAttributeModifier_UE::Add
                (
                    AttributeEntity,
                    ck::FAttributeModifier_ReplicationTags::Get_FinalTag(),
                    FCk_Fragment_FloatAttributeModifier_ParamsData
                    {
                        AttributeToReplicate.Get_Final() - AttributeToReplicate.Get_Base(),
                        ECk_ArithmeticOperations_Basic::Add,
                        ECk_ModifierOperation_RevocablePolicy::Revocable,
                        AttributeToReplicate.Get_Component()
                    }
                );
            }
            else
            {
                UCk_Utils_FloatAttributeModifier_UE::Override(
                    AttributeModifier, AttributeToReplicate.Get_Final() - AttributeToReplicate.Get_Base(), AttributeToReplicate.Get_Component());
            }

            continue;
        }

        ck::attribute::Verbose(TEXT("IGNORING FLOAT Attribute [{}] as there is no change between [{}] and [{}]"),
            AttributeToReplicate.Get_AttributeName());
    }

    _AttributesToReplicate_Previous = _AttributesToReplicate;
}

// --------------------------------------------------------------------------------------------------------------------
