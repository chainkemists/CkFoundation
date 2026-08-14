#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkCVar/CkCVar_Data.h"

#include <GameplayTagContainer.h>

#include "CkGameSettings_Common.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_GameSettings_ValueType : uint8
{
    Bool = 0,
    Int32,
    Float,
    String
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_GameSettings_ValueType);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_GameSettings_Scope : uint8
{
    Machine = 0,
    Player
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_GameSettings_Scope);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_GameSettings_PersistencePolicy : uint8
{
    Provider = 0,
    External
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_GameSettings_PersistencePolicy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_GameSettings_ApplyBindingType : uint8
{
    None = 0,
    CVar,
    Handler
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_GameSettings_ApplyBindingType);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGAMESETTINGS_API FCk_GameSettings_SettingOption
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_GameSettings_SettingOption);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FText _Label;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FString _Value;

public:
    CK_PROPERTY_GET(_Label);
    CK_PROPERTY_GET(_Value);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_GameSettings_SettingOption, _Label, _Value);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGAMESETTINGS_API FCk_GameSettings_EditConditions
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_GameSettings_EditConditions);

private:
    /** Platform traits that must ALL be present for the setting to be editable. Empty = always editable. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _RequiredPlatformTraits;

    /** Player-facing reason shown by setting widgets when the setting is not editable. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FText _DisabledReason;

public:
    CK_PROPERTY(_RequiredPlatformTraits);
    CK_PROPERTY(_DisabledReason);
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Everything describing ONE user-facing game setting. Registered with UCk_GameSettings_Subsystem_UE
 * (directly or through a UCk_GameSettings_Collection_PDA); values are stored canonically as strings
 * and converted at the typed access boundary.
 */
USTRUCT(BlueprintType)
struct CKGAMESETTINGS_API FCk_GameSettings_SettingDefinition
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_GameSettings_SettingDefinition);

private:
    /** Hierarchical setting key, e.g. "audio.volume.master". */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FName _Key;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_GameSettings_ValueType _ValueType = ECk_GameSettings_ValueType::Bool;

    /** Must be parseable as _ValueType — registration rejects the definition otherwise. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FString _DefaultValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_GameSettings_Scope _Scope = ECk_GameSettings_Scope::Machine;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_GameSettings_PersistencePolicy _PersistencePolicy = ECk_GameSettings_PersistencePolicy::Provider;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_GameSettings_ApplyBindingType _ApplyBindingType = ECk_GameSettings_ApplyBindingType::None;

    /** Only meaningful when _ApplyBindingType == CVar. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_CVarRef _CVar;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FText _DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FText _Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _CategoryTags;

    /** Inclusive lower bound. Empty = unbounded. Numeric value types only. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FString _MinValue;

    /** Inclusive upper bound. Empty = unbounded. Numeric value types only. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FString _MaxValue;

    /** Non-empty = the value must match one of these options (a Select-style setting). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_GameSettings_SettingOption> _Options;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_GameSettings_EditConditions _EditConditions;

public:
    CK_PROPERTY_GET(_Key);
    CK_PROPERTY_GET(_ValueType);
    CK_PROPERTY_GET(_DefaultValue);
    CK_PROPERTY(_Scope);
    CK_PROPERTY(_PersistencePolicy);
    CK_PROPERTY(_ApplyBindingType);
    CK_PROPERTY(_CVar);
    CK_PROPERTY(_DisplayName);
    CK_PROPERTY(_Description);
    CK_PROPERTY(_CategoryTags);
    CK_PROPERTY(_MinValue);
    CK_PROPERTY(_MaxValue);
    CK_PROPERTY(_Options);
    CK_PROPERTY(_EditConditions);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_GameSettings_SettingDefinition, _Key, _ValueType, _DefaultValue);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGAMESETTINGS_API FCk_Request_GameSettings_SetValue_Bool
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_GameSettings_SetValue_Bool);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FName _Key;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    bool _Value = false;

public:
    CK_PROPERTY_GET(_Key);
    CK_PROPERTY_GET(_Value);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_GameSettings_SetValue_Bool, _Key, _Value);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGAMESETTINGS_API FCk_Request_GameSettings_SetValue_Int32
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_GameSettings_SetValue_Int32);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FName _Key;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _Value = 0;

public:
    CK_PROPERTY_GET(_Key);
    CK_PROPERTY_GET(_Value);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_GameSettings_SetValue_Int32, _Key, _Value);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGAMESETTINGS_API FCk_Request_GameSettings_SetValue_Float
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_GameSettings_SetValue_Float);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FName _Key;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _Value = 0.0f;

public:
    CK_PROPERTY_GET(_Key);
    CK_PROPERTY_GET(_Value);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_GameSettings_SetValue_Float, _Key, _Value);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGAMESETTINGS_API FCk_Request_GameSettings_SetValue_String
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_GameSettings_SetValue_String);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FName _Key;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FString _Value;

public:
    CK_PROPERTY_GET(_Key);
    CK_PROPERTY_GET(_Value);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_GameSettings_SetValue_String, _Key, _Value);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGAMESETTINGS_API FCk_Request_GameSettings_ResetToDefault
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_GameSettings_ResetToDefault);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FName _Key;

public:
    CK_PROPERTY_GET(_Key);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_GameSettings_ResetToDefault, _Key);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_GameSettings_OnSettingChanged,
    FName, InKey,
    const FString&, InNewValue);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_GameSettings_OnSettingChanged_MC,
    FName, InKey,
    const FString&, InNewValue);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_OneParam(FCk_Delegate_GameSettings_ApplyHandler_Bool, bool, InNewValue);
DECLARE_DYNAMIC_DELEGATE_OneParam(FCk_Delegate_GameSettings_ApplyHandler_Int32, int32, InNewValue);
DECLARE_DYNAMIC_DELEGATE_OneParam(FCk_Delegate_GameSettings_ApplyHandler_Float, float, InNewValue);
DECLARE_DYNAMIC_DELEGATE_OneParam(FCk_Delegate_GameSettings_ApplyHandler_String, const FString&, InNewValue);

DECLARE_DYNAMIC_DELEGATE_RetVal(bool, FCk_Delegate_GameSettings_ExternalGetter_Bool);
DECLARE_DYNAMIC_DELEGATE_RetVal(int32, FCk_Delegate_GameSettings_ExternalGetter_Int32);
DECLARE_DYNAMIC_DELEGATE_RetVal(float, FCk_Delegate_GameSettings_ExternalGetter_Float);
DECLARE_DYNAMIC_DELEGATE_RetVal(FString, FCk_Delegate_GameSettings_ExternalGetter_String);

// --------------------------------------------------------------------------------------------------------------------
