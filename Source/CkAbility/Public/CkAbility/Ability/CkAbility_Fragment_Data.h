#pragma once

#include "CkCore/Public/CkCore/Format/CkFormat.h"
#include "CkCore/Types/DataAsset/CkDataAsset.h"

#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"

#include "CkNet/Public/CkNet/CkNet_Common.h"

#include "CkUnreal/Public/CkUnreal/EntityBridge/CkEntityBridge_Fragment_Data.h"

#include "CkAbility_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Ability_ActivationPolicy : uint8
{
    ActivateManually,
    ActivateOnGranted
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Ability_ActivationPolicy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Ability_Status : uint8
{
    NotActive,
    Active
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Ability_Status);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Ability_ActivationSettings_OnOwner
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ability_ActivationSettings_OnOwner);

private:
    // While this Ability is executing, the owner of the Ability will be granted this set of Tags.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _GrantTagsOnAbilityOwner;

    // The Ability will be cancelled as soon as the AbilityOwner has any of these Tags.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _CancelledByTagsOnAbilityOwner;

    // The Ability can only be activated if the AbilityOwner has all of these Tags.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _RequiredTagsOnAbilityOwner;

    // The Ability can only be activated if the AbilityOwner does not have any of these Tags.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _BlockedByTagsOnAbilityOwner;

public:
    CK_PROPERTY(_GrantTagsOnAbilityOwner);
    CK_PROPERTY(_CancelledByTagsOnAbilityOwner);
    CK_PROPERTY(_RequiredTagsOnAbilityOwner);
    CK_PROPERTY(_BlockedByTagsOnAbilityOwner);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Ability_ActivationSettings_OnSelf
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ability_ActivationSettings_OnSelf);

private:
    // If this Ability is also an AbilityOwner, then this Ability will be cancelled as soon as the Ability has any of these Tags.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _CancelledByTagsOnSelf;

    // If this Ability is also an AbilityOwner, then this Ability can only be activated if the Ability does not have any of these Tags.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _BlockedByTagsOnSelf;

public:
    CK_PROPERTY(_CancelledByTagsOnSelf);
    CK_PROPERTY(_BlockedByTagsOnSelf);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Ability_ActivationSettings
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ability_ActivationSettings);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Ability_ActivationPolicy _ActivationPolicy = ECk_Ability_ActivationPolicy::ActivateManually;

    // Tags on AbilityOwner that affect this Ability
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Ability_ActivationSettings_OnOwner _ActivationSettingsOnOwner;

    // Tags on Ability itself that affect this Ability (only applicable if Ability itself is an AbilityOwner)
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Ability_ActivationSettings_OnSelf _ActivationSettingsOnSelf;

public:
    CK_PROPERTY(_ActivationPolicy);
    CK_PROPERTY(_ActivationSettingsOnOwner);
    CK_PROPERTY(_ActivationSettingsOnSelf);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Ability_NetworkSettings
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ability_NetworkSettings);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
     ECk_Net_ReplicationType _ReplicationType = ECk_Net_ReplicationType::LocalAndHost;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    ECk_Net_NetExecutionPolicy _ExecutionPolicy = ECk_Net_NetExecutionPolicy::PreferHost;

public:
    CK_PROPERTY(_ReplicationType);
    CK_PROPERTY(_ExecutionPolicy);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Ability_Script_Data
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ability_Script_Data);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "Name",
              meta = (AllowPrivateAccess = true))
    FGameplayTag _AbilityName = FGameplayTag::EmptyTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "Activation",
              meta = (AllowPrivateAccess = true))
    FCk_Ability_ActivationSettings _ActivationSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "Replication",
              meta = (AllowPrivateAccess = true))
    FCk_Ability_NetworkSettings _NetworkSettings;

public:
    CK_PROPERTY_GET(_AbilityName);
    CK_PROPERTY(_ActivationSettings);
    CK_PROPERTY(_NetworkSettings);

    CK_DEFINE_CONSTRUCTORS(FCk_Ability_Script_Data, _AbilityName);
};

// --------------------------------------------------------------------------------------------------------------------

// This Object itself is NOT replicated. It may be 'implicitly' replicated through the Ability's replicated fragment
// The interface is purposefully similar to GAS' Gameplay Ability.
// We chose to use the term 'Activate/Deactivate Ability' as opposed to 'Start/End Ability' since it evokes a more immediate process
UCLASS(Abstract, EditInlineNew, Blueprintable, BlueprintType)
class CKABILITY_API UCk_Ability_Script_PDA : public UCk_DataAsset_PDA
{
    GENERATED_BODY()

    friend class UCk_Utils_Ability_UE;

public:
    CK_GENERATED_BODY(UCk_Ability_Script_PDA);

public:
    auto
    OnGiveAbility() -> void;

    auto
    OnRevokeAbility() -> void;

    auto
    OnActivateAbility() -> void;

    auto
    OnDeactivateAbility() -> void;

    auto
    Get_CanActivateAbility() const -> bool;

protected:
    UFUNCTION(BlueprintNativeEvent,
              Category = "Ck|Ability|Script",
              meta     = (DisplayName = "OnActivateAbility"))
    void
    DoOnActivateAbility();

    UFUNCTION(BlueprintNativeEvent,
              Category = "Ck|Ability|Script",
              meta     = (DisplayName = "OnDeactivateAbility"))
    void
    DoOnDeactivateAbility();

    UFUNCTION(BlueprintNativeEvent,
              Category = "Ck|Ability|Script",
              meta     = (DisplayName = "OnGiveAbility"))
    void
    DoOnGiveAbility();

    UFUNCTION(BlueprintNativeEvent,
              Category = "Ck|Ability|Script",
              meta     = (DisplayName = "OnRevokeAbility"))
    void
    DoOnRevokeAbility();

    UFUNCTION(BlueprintNativeEvent,
              Category = "Ck|Ability|Script",
              meta     = (DisplayName = "Get_CanActivateAbility"))
    bool
    DoGet_CanActivateAbility() const;

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Ability|Script",
              meta = (CompactNodeTitle="ACTIVATE_ThisAbility", DefaultToSelf="InScript", HidePin="InScript"))
    static void
    Self_Request_ActivateAbility(
        const UCk_Ability_Script_PDA* InScript);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Ability|Script",
              meta = (CompactNodeTitle="DEACTIVATE_ThisAbility", DefaultToSelf="InScript", HidePin="InScript"))
    static void
    Self_Request_DeactivateAbility(
        const UCk_Ability_Script_PDA* InScript);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Ability|Script",
              meta = (CompactNodeTitle="TRACK_Task", DefaultToSelf="InScript", HidePin="InScript"))
    static void
    Self_Request_TrackTask(
        UCk_Ability_Script_PDA* InScript,
        UObject* InTask);

private:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Ability|Script",
              meta = (CompactNodeTitle="STATUS_ThisAbility", DefaultToSelf="InScript", HidePin="InScript"))
    static ECk_Ability_Status
    Self_Get_Status(
        const UCk_Ability_Script_PDA* InScript);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Ability|Script",
              meta = (CompactNodeTitle="AbilityEntity", DefaultToSelf="InScript", HidePin="InScript"))
    static FCk_Handle
    Self_Get_AbilityEntity(
        const UCk_Ability_Script_PDA* InScript);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Ability|Script",
              meta = (CompactNodeTitle="AbilityOwnerEntity", DefaultToSelf="InScript", HidePin="InScript"))
    static FCk_Handle
    Self_Get_AbilityOwnerEntity(
        const UCk_Ability_Script_PDA* InScript);

private:
    UPROPERTY(EditDefaultsOnly,
              meta = (ShowOnlyInnerProperties))
    FCk_Ability_Script_Data _Data;

private:
    UPROPERTY(Transient)
    FCk_Handle _AbilityHandle;

    UPROPERTY(Transient)
    FCk_Handle _AbilityOwnerHandle;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UObject>> _Tasks;

public:
    CK_PROPERTY_GET(_Data);
    CK_PROPERTY_GET(_AbilityHandle);
    CK_PROPERTY_GET(_AbilityOwnerHandle);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Fragment_Ability_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Ability_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "Script",
              meta = (AllowPrivateAccess = true, AllowAbstract = false))
    TSubclassOf<UCk_Ability_Script_PDA> _AbilityScriptClass;

public:
    CK_PROPERTY_GET(_AbilityScriptClass);

public:
    [[nodiscard]]
    auto
    Get_Data() const -> const FCk_Ability_Script_Data&;

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_Ability_ParamsData, _AbilityScriptClass);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Fragment_MultipleAbility_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_MultipleAbility_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Fragment_Ability_ParamsData> _AbilityParams;

public:
    CK_PROPERTY_GET(_AbilityParams)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MultipleAbility_ParamsData, _AbilityParams);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, EditInlineNew, Blueprintable, BlueprintType)
class CKABILITY_API UCk_Ability_ConstructionScript_PDA : public UCk_Entity_ConstructionScript_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Ability_ConstructionScript_PDA);

public:
    auto DoConstruct_Implementation(
        const FCk_Handle& InHandle) -> void override;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "Ck|Config",
              meta = (ExposeOnSpawn, AllowPrivateAccess = true))
    FCk_Fragment_Ability_ParamsData _AbilityParams;

public:
    CK_PROPERTY_GET(_AbilityParams);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, EditInlineNew, Blueprintable, BlueprintType)
class CKABILITY_API UCk_Ability_EntityConfig_PDA : public UCk_EntityBridge_Config_Base_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Ability_EntityConfig_PDA);

private:
    auto
    DoGet_EntityConstructionScript() const -> UCk_Entity_ConstructionScript_PDA* override;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced,
              meta = (AllowPrivateAccess = true, ExposeOnSpawn = true))
    TObjectPtr<UCk_Ability_ConstructionScript_PDA> _EntityConstructionScript;

public:
    CK_PROPERTY_GET(_EntityConstructionScript);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Ability_OnEnded,
    FCk_Handle, InAbilityHandle,
    FCk_Handle, InAbilityOwnerHandle);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_Ability_OnEnded_MC,
    FCk_Handle, InAbilityHandle,
    FCk_Handle, InAbilityOwnerHandle);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_Ability_OnActivated,
    FCk_Handle, InAbilityHandle);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FCk_Delegate_Ability_OnActivated_MC,
    FCk_Handle, InAbilityHandle);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_Ability_OnDeactivated,
    FCk_Handle, InAbilityHandle);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FCk_Delegate_Ability_OnDeactivated_MC,
    FCk_Handle, InAbilityHandle);

// --------------------------------------------------------------------------------------------------------------------
// Formatters and IsValid

CK_DEFINE_CUSTOM_IS_VALID(FCk_Fragment_Ability_ParamsData, ck::IsValid_Policy_Default,
[=](const FCk_Fragment_Ability_ParamsData& InAbilityParams)
{
    if (ck::Is_NOT_Valid(InAbilityParams.Get_Data().Get_AbilityName()))
    { return false; }

    return true;
});

// --------------------------------------------------------------------------------------------------------------------
