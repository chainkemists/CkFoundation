#include "CkAbility_Script.h"

#include "CkAbility/Ability/CkAbility_Utils.h"
#include "CkAbility/AbilityCue/CkAbilityCue_Utils.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Utils.h"
#include "CkAbility/Settings/CkAbility_Settings.h"

#include "CkCore/Object/CkObject_Utils.h"

#include "CkNet/CkNet_Utils.h"

#include <BlueprintTaskTemplate.h>
#include <NativeGameplayTags.h>
#include <Windows.ApplicationModel.Activation.h>

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
    UCk_Ability_Trait_UE::
    AllowMultipleCopiesInAbilityScript() const
    -> bool
{
    return DoAllowMultipleCopiesInAbilityScript();
}

auto
    UCk_Ability_Trait_UE::
    OnOwningAbilityCreated(
        FCk_Handle_Ability& InAbility)
    -> void
{
    DoOnOwningAbilityCreated(InAbility);
}

auto
    UCk_Ability_Trait_UE::
    OnOwningAbilityGiven(
        FCk_Handle_Ability& InAbility,
        FCk_Handle_AbilityOwner& InAbilityOwner)
    -> void
{
    DoOnOwningAbilityGiven(InAbility, InAbilityOwner);
}

auto
    UCk_Ability_Trait_UE::
    OnOwningAbilityActivated(
        FCk_Handle_Ability& InAbility,
        FCk_Handle_AbilityOwner& InAbilityOwner)
    -> void
{
    DoOnOwningAbilityActivated(InAbility, InAbilityOwner);
}

auto
    UCk_Ability_Trait_UE::
    OnOwningAbilityDeactivated(
        FCk_Handle_Ability& InAbility,
        FCk_Handle_AbilityOwner& InAbilityOwner)
    -> void
{
    DoOnOwningAbilityDeactivated(InAbility, InAbilityOwner);
}

auto
    UCk_Ability_Trait_UE::
    OnOwningAbilityRevoked(
        FCk_Handle_Ability& InAbility,
        FCk_Handle_AbilityOwner& InAbilityOwner)
    -> void
{
    DoOnOwningAbilityRevoked(InAbility, InAbilityOwner);
}

auto
    UCk_Ability_Trait_UE::
    DoOnOwningAbilityCreated_Implementation(
        FCk_Handle_Ability InAbility)
    -> void
{
}

auto
    UCk_Ability_Trait_UE::
    DoOnOwningAbilityGiven_Implementation(
        FCk_Handle_Ability InAbility,
        FCk_Handle_AbilityOwner InAbilityOwner)
    -> void
{
}

auto
    UCk_Ability_Trait_UE::
    DoOnOwningAbilityActivated_Implementation(
        FCk_Handle_Ability InAbility,
        FCk_Handle_AbilityOwner InAbilityOwner)
    -> void
{
}

auto
    UCk_Ability_Trait_UE::
    DoOnOwningAbilityDeactivated_Implementation(
        FCk_Handle_Ability InAbility,
        FCk_Handle_AbilityOwner InAbilityOwner)
    -> void
{
}

auto
    UCk_Ability_Trait_UE::
    DoOnOwningAbilityRevoked_Implementation(
        FCk_Handle_Ability InAbility,
        FCk_Handle_AbilityOwner InAbilityOwner)
    -> void
{
}

auto
    UCk_Ability_Trait_UE::
    DoAllowMultipleCopiesInAbilityScript_Implementation() const
    -> bool
{
    return false;
}

auto
    UCk_Ability_Trait_UE::
    DoGet_OwningAbilityEntity() const
    -> FCk_Handle_Ability
{
    return Get_OwningAbilityHandle();
}

auto
    UCk_Ability_Trait_UE::
    DoGet_OwningAbilityOwnerEntity() const
    -> FCk_Handle_AbilityOwner
{
    return UCk_Utils_Ability_UE::TryGet_Owner(Get_OwningAbilityHandle());;
}

auto
    UCk_Ability_Trait_UE::
    DoRequest_ActivateOwningAbility(
        FCk_Ability_Payload_OnActivate InActivationPayload)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(Get_OwningAbilityHandle()),
        TEXT("Ability Handle stored inside Trait is [{}]. Cannot Activate Owning Ability.{}"),
        Get_OwningAbilityHandle(), ck::Context(this))
    { return; }

    auto AbilityOwner = UCk_Utils_Ability_UE::TryGet_Owner(Get_OwningAbilityHandle());

    UCk_Utils_AbilityOwner_UE::Request_TryActivateAbility(
        AbilityOwner,
        FCk_Request_AbilityOwner_ActivateAbility{Get_OwningAbilityHandle()}.Set_OptionalPayload(InActivationPayload),
        {});
}

auto
    UCk_Ability_Trait_UE::
    DoRequest_DeactivateOwningAbility()
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(Get_OwningAbilityHandle()),
        TEXT("Ability Handle stored inside Trait is [{}]. Cannot Deactivate Owning Ability.{}"),
        Get_OwningAbilityHandle(), ck::Context(this))
    { return; }

    auto AbilityOwner = UCk_Utils_Ability_UE::TryGet_Owner(Get_OwningAbilityHandle());

    UCk_Utils_AbilityOwner_UE::Request_DeactivateAbility(
        AbilityOwner,
        FCk_Request_AbilityOwner_DeactivateAbility{Get_OwningAbilityHandle()},
        {});
}

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR
auto
    UCk_Ability_Script_PDA::
    PostEditChangeChainProperty(
        FPropertyChangedChainEvent& PropertyChangedChainEvent)
    -> void
{
    Super::PostEditChangeChainProperty(PropertyChangedChainEvent);

    const auto& PropertyName = PropertyChangedChainEvent.GetPropertyName();

    if (PropertyName != GET_MEMBER_NAME_CHECKED(FCk_Ability_Script_Data, _AbilityTraits))
    { return; }

    auto& AbilityTraits = _Data._AbilityTraits;
    const auto& AbilityTraitIndexFromProperty = PropertyChangedChainEvent.GetArrayIndex(PropertyName.ToString());

    if (NOT AbilityTraits.IsValidIndex(AbilityTraitIndexFromProperty))
    { return; }

    const auto& EditingTraitObject = AbilityTraits[AbilityTraitIndexFromProperty];

    if (ck::Is_NOT_Valid(EditingTraitObject))
    { return; }

    if (EditingTraitObject->AllowMultipleCopiesInAbilityScript())
    { return; }

    for (auto OtherObjectIndex = 0; OtherObjectIndex < AbilityTraits.Num(); ++OtherObjectIndex)
    {
        const auto& OtherTraitObject = AbilityTraits[OtherObjectIndex];

        if (ck::Is_NOT_Valid(OtherTraitObject))
        { continue; }

        if (EditingTraitObject != OtherTraitObject && EditingTraitObject->GetClass() == OtherTraitObject->GetClass())
        {
            UCk_Utils_EditorOnly_UE::Request_PushNewEditorMessage
            (
                FCk_Utils_EditorOnly_PushNewEditorMessage_Params
                {
                    TEXT("CkAbility"),
                    FCk_MessageSegments
                    {
                        {
                            FCk_TokenizedMessage
                            {
                                ck::Format_UE(TEXT("Ability Trait [{}] does NOT support multiple copied of itself"), EditingTraitObject)
                            }
                            .Set_TargetObject(this)
                        }
                    }
                }
                .Set_MessageSeverity(ECk_EditorMessage_Severity::Error)
                .Set_MessageLogDisplayPolicy(ECk_EditorMessage_MessageLog_DisplayPolicy::DoNotFocus)
                .Set_ToastNotificationDisplayPolicy(ECk_EditorMessage_ToastNotification_DisplayPolicy::Display)
            );

            AbilityTraits[AbilityTraitIndexFromProperty] = nullptr;

            return;
        }
    }
}
#endif

auto
    UCk_Ability_Script_PDA::
    OnActivateAbility(
        const FCk_Ability_Payload_OnActivate& InActivationPayload)
    -> void
{
    DoDebugSet_Activated();

    const auto& AbilityOwnerEntity = Get_AbilityOwnerHandle();

    if (const auto& NetworkSettings = Get_Data().Get_NetworkSettings();
        NetworkSettings.Get_ReplicationType() == ECk_Net_ReplicationType::LocalAndHost)
    {
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
            default:
            {
                CK_INVALID_ENUM(ExecutionPolicy);
                break;
            }
        }
    }
    else
    {
        DoOnActivateAbility(InActivationPayload);
    }
}

auto
    UCk_Ability_Script_PDA::
    OnReactivateAbility(
        const FCk_Ability_Payload_OnActivate& InOptionalPayload)
    -> void
{
    const auto& AbilityOwnerEntity = Get_AbilityOwnerHandle();

    if (const auto& NetworkSettings = Get_Data().Get_NetworkSettings();
        NetworkSettings.Get_ReplicationType() == ECk_Net_ReplicationType::LocalAndHost)
    {
        switch(const auto ExecutionPolicy = Get_Data().Get_NetworkSettings().Get_ExecutionPolicy())
        {
            case ECk_Net_NetExecutionPolicy::PreferHost:
            {
                if (UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(AbilityOwnerEntity))
                {
                    DoOnReactivateAbility(InOptionalPayload);
                }
                break;
            }
            case ECk_Net_NetExecutionPolicy::LocalAndHost:
            {
                if (UCk_Utils_Net_UE::Get_HasAuthority(AbilityOwnerEntity) ||
                    UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(AbilityOwnerEntity))
                {
                    DoOnReactivateAbility(InOptionalPayload);
                }
                break;
            }
            default:
            {
                CK_INVALID_ENUM(ExecutionPolicy);
                break;
            }
        }
    }
    else
    {
        DoOnReactivateAbility(InOptionalPayload);
    }
}

auto
    UCk_Ability_Script_PDA::
    OnDeactivateAbility()
    -> void
{
    DoDebugSet_Deactivated();
    const auto AbilityOwnerEntity = Get_AbilityOwnerHandle();

    ck::algo::ForEachIsValid(_TasksToDeactivateOnDeactivate, [](const TWeakObjectPtr<UBlueprintTaskTemplate>& InTask)
    {
        InTask->Deactivate();
    });

    if (const auto& NetworkSettings = Get_Data().Get_NetworkSettings();
        NetworkSettings.Get_ReplicationType() == ECk_Net_ReplicationType::LocalAndHost)
    {
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
            default:
            {
                CK_INVALID_ENUM(ExecutionPolicy);
                break;
            }
        }
    }
    else
    {
        DoOnDeactivateAbility();
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

    ck::algo::ForEachIsValid(_TasksToDeactivateOnRevoke, [](const TWeakObjectPtr<UBlueprintTaskTemplate>& InTask)
    {
        InTask->Deactivate();
    });

    DoOnRevokeAbility();
}

auto
    UCk_Ability_Script_PDA::
    OnAbilityNotActivated(
        const FCk_Ability_NotActivated_Info& InInfo)
    -> void
{
    if (UCk_Utils_Ability_Settings_UE::Get_AbilityNotActivatedDebug() == ECk_EnableDisable::Disable)
    { return; }

    DoOnAbilityNotActivated(InInfo);
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
    DoRequest_SpawnAbilityCue_Local(
        const FCk_AbilityCue_Params& InReplicatedParams,
        FGameplayTag InAbilityCueName)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InAbilityCueName), TEXT("Invalid AbilityCueName in Ability [{}]"), this)
    { return; }

    UCk_Utils_AbilityCue_UE::Request_Spawn_AbilityCue_Local
    (
        Get_AbilityHandle(),
        FCk_Request_AbilityCue_Spawn{InAbilityCueName, this}
            .Set_ReplicatedParams(InReplicatedParams)
    );
}

auto
    UCk_Ability_Script_PDA::
    DoRequest_AddTaskToDeactivateOnRevoke(
        UBlueprintTaskTemplate* InTask)
    -> void
{
    _TasksToDeactivateOnRevoke.Emplace(InTask);
}

auto
    UCk_Ability_Script_PDA::
    DoRequest_AddTaskToDeactivateOnDeactivate(
        UBlueprintTaskTemplate* InTask)
    -> void
{
    _TasksToDeactivateOnDeactivate.Emplace(InTask);
}

auto
    UCk_Ability_Script_PDA::
    DoGet_Status() const
    -> ECk_Ability_Status
{
    if (_Data.Get_InstancingPolicy() != ECk_Ability_InstancingPolicy::NotInstanced)
    {
        CK_ENSURE_IF_NOT(NOT UCk_Utils_Object_UE::Get_IsDefaultObject(this),
            TEXT("Cannot call UCk_Ability_Script_PDA::DoGet_Status on the CDO of the AbilityScript with InstancingPolicy [{}].\n"
                 "This is because the necessary Ability and AbilityOwner Handles will always be INVALID.{}"),
            ck::Context(this))
        { return {}; }
    }

    CK_ENSURE_IF_NOT(ck::IsValid(Get_AbilityHandle()),
        TEXT("AbilityHandle is [{}]. It's possible that this was not set correctly by the Processor that Gives the Ability.{}"),
        Get_AbilityHandle(), ck::Context(this))
    { return {}; }

    return UCk_Utils_Ability_UE::Get_Status(Get_AbilityHandle());
}

auto
    UCk_Ability_Script_PDA::
    DoGet_AbilityEntity() const
    -> FCk_Handle_Ability
{
    if (_Data.Get_InstancingPolicy() != ECk_Ability_InstancingPolicy::NotInstanced)
    {
        CK_ENSURE_IF_NOT(NOT UCk_Utils_Object_UE::Get_IsDefaultObject(this),
            TEXT("Cannot call UCk_Ability_Script_PDA::DoGet_AbilityEntity on the CDO of the AbilityScript with InstancingPolicy [{}].\n"
                 "This is because the necessary Ability and AbilityOwner Handles will always be INVALID.{}"),
            _Data.Get_InstancingPolicy(), ck::Context(this))
        { return {}; }
    }

    CK_ENSURE_IF_NOT(ck::IsValid(Get_AbilityHandle(), ck::IsValid_Policy_IncludePendingKill{}),
        TEXT("AbilityHandle is [{}]. It's possible that this was not set correctly by the Processor that Gives the Ability.{}"),
        Get_AbilityHandle(), ck::Context(this))
    { return {}; }

    return Get_AbilityHandle();
}

auto
    UCk_Ability_Script_PDA::
    DoGet_AbilityOwnerEntity() const
    -> FCk_Handle_AbilityOwner
{
    if (_Data.Get_InstancingPolicy() != ECk_Ability_InstancingPolicy::NotInstanced)
    {
        CK_ENSURE_IF_NOT(NOT UCk_Utils_Object_UE::Get_IsDefaultObject(this),
            TEXT("Cannot call UCk_Ability_Script_PDA::DoGet_AbilityOwnerEntity on the CDO of the AbilityScript with InstancingPolicy [{}].\n"
                 "This is because the necessary Ability and AbilityOwner Handles will always be INVALID.{}"),
            _Data.Get_InstancingPolicy(), ck::Context(this))
        { return {}; }
    }

    CK_ENSURE_IF_NOT(ck::IsValid(Get_AbilityOwnerHandle(), ck::IsValid_Policy_IncludePendingKill{}),
        TEXT("AbilityOwnerHandle is [{}]. It's possible that this was not set correctly by the Processor that Gives the Ability.{}"),
        Get_AbilityOwnerHandle(), ck::Context(this))
    { return {}; }

    return Get_AbilityOwnerHandle();
}

auto
    UCk_Ability_Script_PDA::
    DoGet_AbilityOwnerEntity_Self() const
    -> FCk_Handle_AbilityOwner
{
    if (_Data.Get_InstancingPolicy() != ECk_Ability_InstancingPolicy::NotInstanced)
    {
        CK_ENSURE_IF_NOT(NOT UCk_Utils_Object_UE::Get_IsDefaultObject(this),
            TEXT("Cannot call UCk_Ability_Script_PDA::DoGet_AbilityOwnerEntity_Self on the CDO of the AbilityScript with InstancingPolicy [{}].\n"
                 "This is because the necessary Ability and AbilityOwner Handles will always be INVALID.{}"),
            _Data.Get_InstancingPolicy(), ck::Context(this))
        { return {}; }
    }

    CK_ENSURE_IF_NOT(UCk_Utils_AbilityOwner_UE::Has(Get_AbilityHandle()), TEXT("AbilityHandle is [{}] which is NOT an AbilityOwner.{}"),
        Get_AbilityHandle(), ck::Context(this))
    { return {}; }

    return UCk_Utils_AbilityOwner_UE::Cast(Get_AbilityHandle());
}

auto
    UCk_Ability_Script_PDA::
    DoGet_ContextEntityWithActor()
    -> FCk_Handle
{
    if (ck::Is_NOT_Valid(_ContextEntityWithActor, ck::IsValid_Policy_OptionalEngagedOnly{}))
    {
        _ContextEntityWithActor = UCk_Utils_OwningActor_UE::TryGet_Entity_OwningActor_InOwnershipChain(DoGet_AbilityEntity());
    }

    return *_ContextEntityWithActor;
}

auto
    UCk_Ability_Script_PDA::
    DoGet_AbilityNetMode(
        ECk_Net_NetModeType& OutAbilityNetMode) const
    -> void
{
    if (_Data.Get_InstancingPolicy() != ECk_Ability_InstancingPolicy::NotInstanced)
    {
        CK_ENSURE_IF_NOT(NOT UCk_Utils_Object_UE::Get_IsDefaultObject(this),
            TEXT("Cannot call UCk_Ability_Script_PDA::DoGet_AbilityNetMode on the CDO of the AbilityScript with InstancingPolicy [{}].\n"
                 "This is because the necessary Ability and AbilityOwner Handles will always be INVALID.{}"),
            _Data.Get_InstancingPolicy(), ck::Context(this))
        { return; }
    }
    CK_ENSURE_IF_NOT(ck::IsValid(Get_AbilityHandle()),
        TEXT("AbilityHandle is [{}]. It's possible that this was not set correctly by the Processor that Gives the Ability.{}"),
        Get_AbilityHandle(), ck::Context(this))
    { return; }

    OutAbilityNetMode = UCk_Utils_Net_UE::Get_EntityNetMode(Get_AbilityHandle());
}

auto
    UCk_Ability_Script_PDA::
    Get_Data() const
    -> const FCk_Ability_Script_Data&
{
    return _Data;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Ability_Script_PDA::
    SyncTagsWithAbilityOwner(
        const FGameplayTagContainer& InRelevantTags)
    -> void
{
    UCk_Utils_AbilityOwner_UE::SyncTagsWithAbilityOwner(_AbilityHandle, _AbilityOwnerHandle, InRelevantTags);
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
