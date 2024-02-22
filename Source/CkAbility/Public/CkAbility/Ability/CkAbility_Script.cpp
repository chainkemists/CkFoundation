#include "CkAbility_Script.h"

#include "CkAbility/Ability/CkAbility_Utils.h"
#include "CkAbility/AbilityCue/CkAbilityCue_Utils.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Utils.h"
#include "CkAbility/Settings/CkAbility_Settings.h"

#include "CkNet/CkNet_Utils.h"

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ReSharper disable once CppPolymorphicClassWithNonVirtualPublicDestructor
    struct FAbility_Tags final : public FGameplayTagNativeAdder
    {
    protected:
        auto AddTags() -> void override
        {
            auto& Manager = UGameplayTagsManager::Get();

            _Cooldown  = Manager.AddNativeGameplayTag(TEXT("Ck.AbilityTrigger.ApplyCooldown"));
            _Cost      = Manager.AddNativeGameplayTag(TEXT("Ck.AbilityTrigger.ApplyCost"));
            _Condition = Manager.AddNativeGameplayTag(TEXT("Ck.Ability.Condition"));

            Manager.AddNativeGameplayTag(TEXT("Ck.Ability.CooldownInProgress"));
            Manager.AddNativeGameplayTag(TEXT("Ck.Ability.CostsNotMet"));
            Manager.AddNativeGameplayTag(TEXT("Ck.Ability.ConditionsNotMet"));

            Manager.AddNativeGameplayTag(TEXT("Ck.AbilityTrigger.ApplyCostsNotMet"));
        }

    private:
        FGameplayTag _Cooldown;
        FGameplayTag _Cost;
        FGameplayTag _Condition;

        static FAbility_Tags _Tags;

    public:
        static auto Get_Cooldown()  -> FGameplayTag { return _Tags._Cooldown; }
        static auto Get_Cost()      -> FGameplayTag { return _Tags._Cost; }
        static auto Get_Condition() -> FGameplayTag { return _Tags._Condition; }
    };

    FAbility_Tags FAbility_Tags::_Tags;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Ability_Script_PDA::
    OnActivateAbility(
        const FCk_Ability_Payload_OnActivate& InActivationPayload)
    -> void
{
    const auto AbilityOwnerEntity = Get_AbilityOwnerHandle();

    switch(const auto ExecutionPolicy = Get_Data().Get_NetworkSettings().Get_ExecutionPolicy())
    {
        case ECk_Net_NetExecutionPolicy::PreferHost:
        {
            if (UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(AbilityOwnerEntity))
            {
                DoOnActivateAbility(InActivationPayload);
            }
            break;
        }
        case ECk_Net_NetExecutionPolicy::LocalAndHost:
        {
            if (UCk_Utils_Net_UE::Get_HasAuthority(AbilityOwnerEntity) ||
                UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(AbilityOwnerEntity))
            {
                DoOnActivateAbility(InActivationPayload);
            }
            break;
        }
        case ECk_Net_NetExecutionPolicy::All:
        {
            DoOnActivateAbility(InActivationPayload);
            break;
        }
        default:
        {
            CK_INVALID_ENUM(ExecutionPolicy);
            break;
        }
    }
}

auto
    UCk_Ability_Script_PDA::
    OnDeactivateAbility()
    -> void
{
    DoOnDeactivateAbility();
}

auto
    UCk_Ability_Script_PDA::
    OnGiveAbility(
        const FCk_Ability_Payload_OnGranted& InOptionalPayload)
    -> void
{
    DoOnGiveAbility(InOptionalPayload);
}

auto
    UCk_Ability_Script_PDA::
    OnRevokeAbility()
    -> void
{
    DoOnRevokeAbility();
}

auto
    UCk_Ability_Script_PDA::
    DoRequest_ActivateAbility(
        FCk_Ability_Payload_OnActivate InOptionalPayload)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(Get_AbilityHandle()),
        TEXT("AbilityHandle is INVALID. It's possible that this was not set correctly by the Processor that Gives the Ability.{}"),
        ck::Context(this))
    { return; }

    UCk_Utils_AbilityOwner_UE::Request_TryActivateAbility(_AbilityOwnerHandle,
        FCk_Request_AbilityOwner_ActivateAbility{Get_AbilityHandle()}.Set_OptionalPayload(InOptionalPayload));
}

auto
    UCk_Ability_Script_PDA::
    DoRequest_DeactivateAbility()
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(Get_AbilityHandle()),
        TEXT("AbilityHandle is INVALID. It's possible that this was not set correctly by the Processor that Gives the Ability.{}"),
        ck::Context(this))
    { return; }

    UCk_Utils_AbilityOwner_UE::Request_DeactivateAbility(_AbilityOwnerHandle,
        FCk_Request_AbilityOwner_DeactivateAbility{Get_AbilityHandle()});
}

auto
    UCk_Ability_Script_PDA::
    DoRequest_ApplyCost()
    -> void
{
    DoRequest_ApplyCustomCost(ck::FAbility_Tags::Get_Cost());
}

auto
    UCk_Ability_Script_PDA::
    DoRequest_ApplyCustomCost(
        FGameplayTag InAbilityCostTag)
    -> void
{
    CK_ENSURE_IF_NOT(UCk_Utils_AbilityOwner_UE::Has(Get_AbilityHandle()),
        TEXT("Ability Entity [{}] with AbilityScript [{}] is NOT an AbilityOwner itself. Did you forget to add a 'Cost' Ability to the aforementioned?"),
        Get_AbilityHandle(), this)
    { return; }

    auto AbilityAsAbilityOwner = UCk_Utils_AbilityOwner_UE::CastChecked(_AbilityHandle);
    UCk_Utils_AbilityOwner_UE::Request_SendAbilityEvent
    (
        AbilityAsAbilityOwner,
        FCk_Request_AbilityOwner_SendEvent
        {
            FCk_AbilityOwner_Event{InAbilityCostTag}
                .Set_ContextEntity(Get_AbilityHandle())
        }
    );
}

auto
    UCk_Ability_Script_PDA::
    DoRequest_ApplyCost_OnOwner()
    -> void
{
    DoRequest_ApplyCustomCost_OnOwner(ck::FAbility_Tags::Get_Cost());
}

auto
    UCk_Ability_Script_PDA::
    DoRequest_ApplyCustomCost_OnOwner(
        FGameplayTag InAbilityCostTag)
    -> void
{
    UCk_Utils_AbilityOwner_UE::Request_SendAbilityEvent
    (
        _AbilityOwnerHandle,
        FCk_Request_AbilityOwner_SendEvent
        {
            FCk_AbilityOwner_Event{InAbilityCostTag}
                .Set_ContextEntity(Get_AbilityHandle())
        }
    );
}

auto
    UCk_Ability_Script_PDA::
    DoRequest_ApplyCooldown()
    -> void
{
    DoRequest_ApplyCustomCooldown(ck::FAbility_Tags::Get_Cooldown());
}

auto
    UCk_Ability_Script_PDA::
    DoRequest_ApplyCustomCooldown(
        FGameplayTag InAbilityCooldownTag)
    -> void
{
    CK_ENSURE_IF_NOT(UCk_Utils_AbilityOwner_UE::Has(Get_AbilityHandle()),
        TEXT("Ability Entity [{}] with AbilityScript [{}] is NOT an AbilityOwner itself. Did you forget to add a 'Cooldown' Ability to the aforementioned?"),
        Get_AbilityHandle(), this)
    { return; }

    auto AbilityAsAbilityOwner = UCk_Utils_AbilityOwner_UE::CastChecked(_AbilityHandle);
    UCk_Utils_AbilityOwner_UE::Request_SendAbilityEvent
    (
        AbilityAsAbilityOwner,
        FCk_Request_AbilityOwner_SendEvent
        {
            FCk_AbilityOwner_Event{InAbilityCooldownTag}
                .Set_ContextEntity(Get_AbilityHandle())
        }
    );
}

auto
    UCk_Ability_Script_PDA::
    DoRequest_ApplyCooldown_OnOwner()
    -> void
{
    DoRequest_ApplyCustomCooldown_OnOwner(ck::FAbility_Tags::Get_Cooldown());
}

auto
    UCk_Ability_Script_PDA::
    DoRequest_ApplyCustomCooldown_OnOwner(
        FGameplayTag InAbilityCooldownTag)
    -> void
{
    UCk_Utils_AbilityOwner_UE::Request_SendAbilityEvent
    (
        _AbilityOwnerHandle,
        FCk_Request_AbilityOwner_SendEvent
        {
            FCk_AbilityOwner_Event{InAbilityCooldownTag}
                .Set_ContextEntity(Get_AbilityHandle())
        }
    );
}

auto
    UCk_Ability_Script_PDA::
    DoRequest_TrackTask(
        UObject* InTask)
    -> void
{
    if (ck::Is_NOT_Valid(InTask))
    { return; }

    _Tasks.Emplace(InTask);
}

auto
    UCk_Ability_Script_PDA::
    DoRequest_SpawnAbilityCue(
        const FCk_AbilityCue_Params&  InReplicatedParams,
        FGameplayTag InAbilityCueName) const
    -> void
{
    UCk_Utils_AbilityCue_UE::Request_Spawn_AbilityCue
    (
        Get_AbilityHandle(),
        FCk_Request_AbilityCue_Spawn{InAbilityCueName, const_cast<UCk_Ability_Script_PDA*>(this)}
            .Set_ReplicatedParams(InReplicatedParams)
    );
}

auto
    UCk_Ability_Script_PDA::
    DoGet_Status() const
    -> ECk_Ability_Status
{
    CK_ENSURE_IF_NOT(ck::IsValid(Get_AbilityHandle()),
        TEXT("AbilityHandle is INVALID. It's possible that this was not set correctly by the Processor that Gives the Ability.{}"),
        ck::Context(this))
    { return {}; }

    return UCk_Utils_Ability_UE::Get_Status(Get_AbilityHandle());
}

auto
    UCk_Ability_Script_PDA::
    DoGet_AbilityEntity() const
    -> FCk_Handle_Ability
{
    CK_ENSURE_IF_NOT(ck::IsValid(Get_AbilityHandle(), ck::IsValid_Policy_IncludePendingKill{}),
        TEXT("AbilityHandle is INVALID. It's possible that this was not set correctly by the Processor that Gives the Ability.{}"),
        ck::Context(this))
    { return {}; }

    return Get_AbilityHandle();
}

auto
    UCk_Ability_Script_PDA::
    DoGet_AbilityOwnerEntity() const
    -> FCk_Handle_AbilityOwner
{
    CK_ENSURE_IF_NOT(ck::IsValid(Get_AbilityOwnerHandle(), ck::IsValid_Policy_IncludePendingKill{}),
        TEXT("AbilityOwnerHandle is INVALID. It's possible that this was not set correctly by the Processor that Gives the Ability.{}"),
        ck::Context(this))
    { return {}; }

    return Get_AbilityOwnerHandle();
}

// --------------------------------------------------------------------------------------------------------------------
