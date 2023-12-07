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
struct CKABILITY_API FCk_Ability_ActivationSettings
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Ability_ActivationSettings);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Ability_ActivationPolicy _ActivationPolicy = ECk_Ability_ActivationPolicy::ActivateManually;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _ActivationOwnerGrantedTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _ActivationCancelledTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _ActivationRequiredTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _ActivationBlockedTags;

public:
    CK_PROPERTY(_ActivationPolicy);
    CK_PROPERTY(_ActivationOwnerGrantedTags);
    CK_PROPERTY(_ActivationCancelledTags);
    CK_PROPERTY(_ActivationRequiredTags);
    CK_PROPERTY(_ActivationBlockedTags);
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
// TODO: Hide necessary categories
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
    OnEndAbility() -> void;

    auto
    Get_CanActivateAbility() const -> bool;

protected:
    UFUNCTION(BlueprintNativeEvent,
              Category = "Ck|Ability|Script",
              meta     = (DisplayName = "OnActivateAbility"))
    void DoOnActivateAbility();

    UFUNCTION(BlueprintNativeEvent,
              Category = "Ck|Ability|Script",
              meta     = (DisplayName = "OnEndAbility"))
    void DoOnEndAbility();

    UFUNCTION(BlueprintNativeEvent,
              Category = "Ck|Ability|Script",
              meta     = (DisplayName = "OnGiveAbility"))
    void DoOnGiveAbility();

    UFUNCTION(BlueprintNativeEvent,
              Category = "Ck|Ability|Script",
              meta     = (DisplayName = "OnRevokeAbility"))
    void DoOnRevokeAbility();

    UFUNCTION(BlueprintNativeEvent,
              Category = "Ck|Ability|Script",
              meta     = (DisplayName = "Get_CanActivateAbility"))
    bool DoGet_CanActivateAbility() const;

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Ability|Script",
              meta = (CompactNodeTitle="AbilityEntity", DefaultToSelf="InScript", HidePin="InScript"))
    static FCk_Handle
     Get_AbilityEntity(const UCk_Ability_Script_PDA* InScript);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Ability|Script",
              meta = (CompactNodeTitle="AbilityOwnerEntity", DefaultToSelf="InScript", HidePin="InScript"))
    static FCk_Handle
     Get_AbilityOwnerEntity(const UCk_Ability_Script_PDA* InScript);


private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        Category = "Ck|Config",
        meta = (AllowPrivateAccess = true))
    FCk_Ability_Script_Data _Data;

    UPROPERTY(Transient)
    FCk_Handle _AbilityHandle;

    UPROPERTY(Transient)
    FCk_Handle _AbilityOwnerHandle;

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
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn, AllowPrivateAccess = true))
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
    FCk_Delegate_GameplayAbility_OnEnded,
    FCk_Handle, InAbilityHandle,
    FCk_Handle, InAbilityOwnerHandle);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_GameplayAbility_OnEnded_MC,
    FCk_Handle, InAbilityHandle,
    FCk_Handle, InAbilityOwnerHandle);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_GameplayAbility_OnActivated,
    FCk_Handle, InAbilityHandle,
    FCk_Handle, InAbilityOwnerHandle);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_GameplayAbility_OnActivated_MC,
    FCk_Handle, InAbilityHandle,
    FCk_Handle, InAbilityOwnerHandle);

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
