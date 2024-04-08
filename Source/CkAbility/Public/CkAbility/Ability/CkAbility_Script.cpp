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
            Manager.AddNativeGameplayTag(TEXT("Ck.Ability.Duration"));
            Manager.AddNativeGameplayTag(TEXT("Ck.Ability.Period"));

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
    DoDebugSet_Activated();

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
    DoDebugSet_Deactivated();
    const auto AbilityOwnerEntity = Get_AbilityOwnerHandle();

    switch(const auto ExecutionPolicy = Get_Data().Get_NetworkSettings().Get_ExecutionPolicy())
    {
        case ECk_Net_NetExecutionPolicy::PreferHost:
        {
            if (UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(AbilityOwnerEntity))
            {
                DoOnDeactivateAbility();
            }
            break;
        }
        case ECk_Net_NetExecutionPolicy::LocalAndHost:
        {
            if (UCk_Utils_Net_UE::Get_HasAuthority(AbilityOwnerEntity) ||
                UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(AbilityOwnerEntity))
            {
                DoOnDeactivateAbility();
            }
            break;
        }
        case ECk_Net_NetExecutionPolicy::All:
        {
            DoOnDeactivateAbility();
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
    Get_CanBeGiven(
        const FCk_Handle_AbilityOwner& InAbilityOwner,
        const FCk_Handle& InAbilitySource) const
    -> bool
{
    return DoGet_CanBeGiven(InAbilityOwner, InAbilitySource);
}

auto
    UCk_Ability_Script_PDA::
    OnNotGiven(
        const FCk_Handle_AbilityOwner& InAbilityOwner,
        const FCk_Handle& InAbilitySource) const
    -> void
{
    DoOnNotGiven(InAbilityOwner, InAbilitySource);
}

auto
    UCk_Ability_Script_PDA::
    OnGiveAbility(
        const FCk_Ability_Payload_OnGranted& InOptionalPayload)
    -> void
{
    DoDebugSet_Given();

    DoOnGiveAbility(InOptionalPayload);
}

auto
    UCk_Ability_Script_PDA::
    OnRevokeAbility()
    -> void
{
    DoDebugSet_Revoked();

    DoOnRevokeAbility();
}

auto
    UCk_Ability_Script_PDA::
    DoRequest_ActivateAbility(
        FCk_Ability_Payload_OnActivate InOptionalPayload)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(Get_AbilityHandle()),
        TEXT("AbilityHandle is [{}]. It's possible that this was not set correctly by the Processor that Gives the Ability.{}"),
        Get_AbilityHandle(), ck::Context(this))
    { return; }

    UCk_Utils_AbilityOwner_UE::Request_TryActivateAbility(
        _AbilityOwnerHandle,
        FCk_Request_AbilityOwner_ActivateAbility{Get_AbilityHandle()}.Set_OptionalPayload(InOptionalPayload),
        {});
}

auto
    UCk_Ability_Script_PDA::
    DoRequest_DeactivateAbility()
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(Get_AbilityHandle()),
        TEXT("AbilityHandle is [{}]. It's possible that this was not set correctly by the Processor that Gives the Ability.{}"),
        Get_AbilityHandle(), ck::Context(this))
    { return; }

    UCk_Utils_AbilityOwner_UE::Request_DeactivateAbility(
        _AbilityOwnerHandle,
        FCk_Request_AbilityOwner_DeactivateAbility{Get_AbilityHandle()},
        {});
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
    if (NOT UCk_Utils_AbilityOwner_UE::Has(Get_AbilityHandle()))
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
    if (NOT UCk_Utils_AbilityOwner_UE::Has(Get_AbilityHandle()))
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
    DoRequest_SpawnAbilityCue(
        const FCk_AbilityCue_Params&  InReplicatedParams,
        FGameplayTag InAbilityCueName)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InAbilityCueName), TEXT("Invalid AbilityCueName in Ability [{}]"), this)
    { return; }

    UCk_Utils_AbilityCue_UE::Request_Spawn_AbilityCue
    (
        Get_AbilityHandle(),
        FCk_Request_AbilityCue_Spawn{InAbilityCueName, this}
            .Set_ReplicatedParams(InReplicatedParams)
    );
}

auto
    UCk_Ability_Script_PDA::
    DoGet_Status()
    -> ECk_Ability_Status
{
    CK_ENSURE_IF_NOT(ck::IsValid(Get_AbilityHandle()),
        TEXT("AbilityHandle is [{}]. It's possible that this was not set correctly by the Processor that Gives the Ability.{}"),
        Get_AbilityHandle(), ck::Context(this))
    { return {}; }

    return UCk_Utils_Ability_UE::Get_Status(Get_AbilityHandle());
}

auto
    UCk_Ability_Script_PDA::
    DoGet_AbilityEntity()
    -> FCk_Handle_Ability
{
    CK_ENSURE_IF_NOT(ck::IsValid(Get_AbilityHandle(), ck::IsValid_Policy_IncludePendingKill{}),
        TEXT("AbilityHandle is [{}]. It's possible that this was not set correctly by the Processor that Gives the Ability.{}"),
        Get_AbilityHandle(), ck::Context(this))
    { return {}; }

    return Get_AbilityHandle();
}

auto
    UCk_Ability_Script_PDA::
    DoGet_AbilityOwnerEntity()
    -> FCk_Handle_AbilityOwner
{
    CK_ENSURE_IF_NOT(ck::IsValid(Get_AbilityOwnerHandle(), ck::IsValid_Policy_IncludePendingKill{}),
        TEXT("AbilityOwnerHandle is [{}]. It's possible that this was not set correctly by the Processor that Gives the Ability.{}"),
        Get_AbilityOwnerHandle(), ck::Context(this))
    { return {}; }

    return Get_AbilityOwnerHandle();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Ability_Script_PDA::
    DoDebugSet_Activated()
    -> void
{
#if NOT CK_DISABLE_ABILITY_SCRIPT_DEBUGGING
    _ActivateDeactivate = EActivatedDeactivated::Activated;
#endif
}

auto
    UCk_Ability_Script_PDA::
    DoDebugSet_Deactivated()
    -> void
{
#if NOT CK_DISABLE_ABILITY_SCRIPT_DEBUGGING
    _ActivateDeactivate = EActivatedDeactivated::Deactivated;
#endif
}

auto
    UCk_Ability_Script_PDA::
    DoDebugSet_Given()
    -> void
{
#if NOT CK_DISABLE_ABILITY_SCRIPT_DEBUGGING
    _GiveRevoke = EGivenRevoked::Given;
#endif
}

auto
    UCk_Ability_Script_PDA::
    DoDebugSet_Revoked()
    -> void
{
#if NOT CK_DISABLE_ABILITY_SCRIPT_DEBUGGING
    _GiveRevoke = EGivenRevoked::Revoked;
#endif
}

// --------------------------------------------------------------------------------------------------------------------
