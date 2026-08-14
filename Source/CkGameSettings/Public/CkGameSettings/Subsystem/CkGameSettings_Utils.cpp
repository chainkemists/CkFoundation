#include "CkGameSettings_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkGameSettings/Subsystem/CkGameSettings_Subsystem.h"

#include <Engine/GameInstance.h>
#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_GameSettings_UE::
    Request_RegisterSetting(
        const UObject* InWorldContextObject,
        const FCk_GameSettings_SettingDefinition& InDefinition)
    -> bool
{
    auto* Subsystem = DoGet_Subsystem(InWorldContextObject);

    if (ck::Is_NOT_Valid(Subsystem))
    { return false; }

    return Subsystem->Request_RegisterSetting(InDefinition);
}

auto
    UCk_Utils_GameSettings_UE::
    Request_RegisterSettings(
        const UObject* InWorldContextObject,
        const TArray<FCk_GameSettings_SettingDefinition>& InDefinitions)
    -> bool
{
    auto* Subsystem = DoGet_Subsystem(InWorldContextObject);

    if (ck::Is_NOT_Valid(Subsystem))
    { return false; }

    return Subsystem->Request_RegisterSettings(InDefinitions);
}

auto
    UCk_Utils_GameSettings_UE::
    Request_RegisterCollection(
        const UObject* InWorldContextObject,
        const UCk_GameSettings_Collection_PDA* InCollection)
    -> bool
{
    auto* Subsystem = DoGet_Subsystem(InWorldContextObject);

    if (ck::Is_NOT_Valid(Subsystem))
    { return false; }

    return Subsystem->Request_RegisterCollection(InCollection);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_GameSettings_UE::
    Get_SettingValue_Bool(
        const UObject* InWorldContextObject,
        FName InKey,
        bool InFallback)
    -> bool
{
    auto* Subsystem = DoGet_Subsystem(InWorldContextObject);

    if (ck::Is_NOT_Valid(Subsystem))
    { return InFallback; }

    return Subsystem->Get_SettingValue_Bool(InKey, InFallback);
}

auto
    UCk_Utils_GameSettings_UE::
    Get_SettingValue_Int32(
        const UObject* InWorldContextObject,
        FName InKey,
        int32 InFallback)
    -> int32
{
    auto* Subsystem = DoGet_Subsystem(InWorldContextObject);

    if (ck::Is_NOT_Valid(Subsystem))
    { return InFallback; }

    return Subsystem->Get_SettingValue_Int32(InKey, InFallback);
}

auto
    UCk_Utils_GameSettings_UE::
    Get_SettingValue_Float(
        const UObject* InWorldContextObject,
        FName InKey,
        float InFallback)
    -> float
{
    auto* Subsystem = DoGet_Subsystem(InWorldContextObject);

    if (ck::Is_NOT_Valid(Subsystem))
    { return InFallback; }

    return Subsystem->Get_SettingValue_Float(InKey, InFallback);
}

auto
    UCk_Utils_GameSettings_UE::
    Get_SettingValue_String(
        const UObject* InWorldContextObject,
        FName InKey,
        const FString& InFallback)
    -> FString
{
    auto* Subsystem = DoGet_Subsystem(InWorldContextObject);

    if (ck::Is_NOT_Valid(Subsystem))
    { return InFallback; }

    return Subsystem->Get_SettingValue_String(InKey, InFallback);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_GameSettings_UE::
    Get_IsSettingRegistered(
        const UObject* InWorldContextObject,
        FName InKey)
    -> bool
{
    auto* Subsystem = DoGet_Subsystem(InWorldContextObject);

    if (ck::Is_NOT_Valid(Subsystem))
    { return false; }

    return Subsystem->Get_IsSettingRegistered(InKey);
}

auto
    UCk_Utils_GameSettings_UE::
    Get_SettingDefinition(
        const UObject* InWorldContextObject,
        FName InKey,
        FCk_GameSettings_SettingDefinition& OutDefinition)
    -> bool
{
    auto* Subsystem = DoGet_Subsystem(InWorldContextObject);

    if (ck::Is_NOT_Valid(Subsystem))
    { return false; }

    return Subsystem->Get_SettingDefinition(InKey, OutDefinition);
}

auto
    UCk_Utils_GameSettings_UE::
    Get_AllSettingKeys(
        const UObject* InWorldContextObject)
    -> TArray<FName>
{
    auto* Subsystem = DoGet_Subsystem(InWorldContextObject);

    if (ck::Is_NOT_Valid(Subsystem))
    { return {}; }

    return Subsystem->Get_AllSettingKeys();
}

auto
    UCk_Utils_GameSettings_UE::
    Get_SettingKeysByCategory(
        const UObject* InWorldContextObject,
        const FGameplayTagQuery& InCategoryQuery)
    -> TArray<FName>
{
    auto* Subsystem = DoGet_Subsystem(InWorldContextObject);

    if (ck::Is_NOT_Valid(Subsystem))
    { return {}; }

    return Subsystem->Get_SettingKeysByCategory(InCategoryQuery);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_GameSettings_UE::
    Request_SetSettingValue_Bool(
        const UObject* InWorldContextObject,
        const FCk_Request_GameSettings_SetValue_Bool& InRequest)
    -> bool
{
    auto* Subsystem = DoGet_Subsystem(InWorldContextObject);

    if (ck::Is_NOT_Valid(Subsystem))
    { return false; }

    return Subsystem->Request_SetSettingValue_Bool(InRequest);
}

auto
    UCk_Utils_GameSettings_UE::
    Request_SetSettingValue_Int32(
        const UObject* InWorldContextObject,
        const FCk_Request_GameSettings_SetValue_Int32& InRequest)
    -> bool
{
    auto* Subsystem = DoGet_Subsystem(InWorldContextObject);

    if (ck::Is_NOT_Valid(Subsystem))
    { return false; }

    return Subsystem->Request_SetSettingValue_Int32(InRequest);
}

auto
    UCk_Utils_GameSettings_UE::
    Request_SetSettingValue_Float(
        const UObject* InWorldContextObject,
        const FCk_Request_GameSettings_SetValue_Float& InRequest)
    -> bool
{
    auto* Subsystem = DoGet_Subsystem(InWorldContextObject);

    if (ck::Is_NOT_Valid(Subsystem))
    { return false; }

    return Subsystem->Request_SetSettingValue_Float(InRequest);
}

auto
    UCk_Utils_GameSettings_UE::
    Request_SetSettingValue_String(
        const UObject* InWorldContextObject,
        const FCk_Request_GameSettings_SetValue_String& InRequest)
    -> bool
{
    auto* Subsystem = DoGet_Subsystem(InWorldContextObject);

    if (ck::Is_NOT_Valid(Subsystem))
    { return false; }

    return Subsystem->Request_SetSettingValue_String(InRequest);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_GameSettings_UE::
    Request_ResetToDefault(
        const UObject* InWorldContextObject,
        const FCk_Request_GameSettings_ResetToDefault& InRequest)
    -> bool
{
    auto* Subsystem = DoGet_Subsystem(InWorldContextObject);

    if (ck::Is_NOT_Valid(Subsystem))
    { return false; }

    return Subsystem->Request_ResetToDefault(InRequest);
}

auto
    UCk_Utils_GameSettings_UE::
    Request_ResetAllToDefaults(
        const UObject* InWorldContextObject)
    -> int32
{
    auto* Subsystem = DoGet_Subsystem(InWorldContextObject);

    if (ck::Is_NOT_Valid(Subsystem))
    { return 0; }

    return Subsystem->Request_ResetAllToDefaults();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_GameSettings_UE::
    BindTo_OnSettingChanged(
        const UObject* InWorldContextObject,
        FName InKey,
        const FCk_Delegate_GameSettings_OnSettingChanged& InDelegate)
    -> void
{
    auto* Subsystem = DoGet_Subsystem(InWorldContextObject);

    if (ck::Is_NOT_Valid(Subsystem))
    { return; }

    Subsystem->BindTo_OnSettingChanged(InKey, InDelegate);
}

auto
    UCk_Utils_GameSettings_UE::
    UnbindFrom_OnSettingChanged(
        const UObject* InWorldContextObject,
        FName InKey,
        const FCk_Delegate_GameSettings_OnSettingChanged& InDelegate)
    -> void
{
    auto* Subsystem = DoGet_Subsystem(InWorldContextObject);

    if (ck::Is_NOT_Valid(Subsystem))
    { return; }

    Subsystem->UnbindFrom_OnSettingChanged(InKey, InDelegate);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_GameSettings_UE::
    Request_RegisterApplyHandler_Bool(
        const UObject* InWorldContextObject,
        FName InKey,
        const FCk_Delegate_GameSettings_ApplyHandler_Bool& InHandler)
    -> bool
{
    auto* Subsystem = DoGet_Subsystem(InWorldContextObject);

    if (ck::Is_NOT_Valid(Subsystem))
    { return false; }

    return Subsystem->Request_RegisterApplyHandler_Bool(InKey, InHandler);
}

auto
    UCk_Utils_GameSettings_UE::
    Request_RegisterApplyHandler_Int32(
        const UObject* InWorldContextObject,
        FName InKey,
        const FCk_Delegate_GameSettings_ApplyHandler_Int32& InHandler)
    -> bool
{
    auto* Subsystem = DoGet_Subsystem(InWorldContextObject);

    if (ck::Is_NOT_Valid(Subsystem))
    { return false; }

    return Subsystem->Request_RegisterApplyHandler_Int32(InKey, InHandler);
}

auto
    UCk_Utils_GameSettings_UE::
    Request_RegisterApplyHandler_Float(
        const UObject* InWorldContextObject,
        FName InKey,
        const FCk_Delegate_GameSettings_ApplyHandler_Float& InHandler)
    -> bool
{
    auto* Subsystem = DoGet_Subsystem(InWorldContextObject);

    if (ck::Is_NOT_Valid(Subsystem))
    { return false; }

    return Subsystem->Request_RegisterApplyHandler_Float(InKey, InHandler);
}

auto
    UCk_Utils_GameSettings_UE::
    Request_RegisterApplyHandler_String(
        const UObject* InWorldContextObject,
        FName InKey,
        const FCk_Delegate_GameSettings_ApplyHandler_String& InHandler)
    -> bool
{
    auto* Subsystem = DoGet_Subsystem(InWorldContextObject);

    if (ck::Is_NOT_Valid(Subsystem))
    { return false; }

    return Subsystem->Request_RegisterApplyHandler_String(InKey, InHandler);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_GameSettings_UE::
    Request_RegisterExternalAccessors_Bool(
        const UObject* InWorldContextObject,
        FName InKey,
        const FCk_Delegate_GameSettings_ExternalGetter_Bool& InGetter,
        const FCk_Delegate_GameSettings_ApplyHandler_Bool& InSetter)
    -> bool
{
    auto* Subsystem = DoGet_Subsystem(InWorldContextObject);

    if (ck::Is_NOT_Valid(Subsystem))
    { return false; }

    return Subsystem->Request_RegisterExternalAccessors_Bool(InKey, InGetter, InSetter);
}

auto
    UCk_Utils_GameSettings_UE::
    Request_RegisterExternalAccessors_Int32(
        const UObject* InWorldContextObject,
        FName InKey,
        const FCk_Delegate_GameSettings_ExternalGetter_Int32& InGetter,
        const FCk_Delegate_GameSettings_ApplyHandler_Int32& InSetter)
    -> bool
{
    auto* Subsystem = DoGet_Subsystem(InWorldContextObject);

    if (ck::Is_NOT_Valid(Subsystem))
    { return false; }

    return Subsystem->Request_RegisterExternalAccessors_Int32(InKey, InGetter, InSetter);
}

auto
    UCk_Utils_GameSettings_UE::
    Request_RegisterExternalAccessors_Float(
        const UObject* InWorldContextObject,
        FName InKey,
        const FCk_Delegate_GameSettings_ExternalGetter_Float& InGetter,
        const FCk_Delegate_GameSettings_ApplyHandler_Float& InSetter)
    -> bool
{
    auto* Subsystem = DoGet_Subsystem(InWorldContextObject);

    if (ck::Is_NOT_Valid(Subsystem))
    { return false; }

    return Subsystem->Request_RegisterExternalAccessors_Float(InKey, InGetter, InSetter);
}

auto
    UCk_Utils_GameSettings_UE::
    Request_RegisterExternalAccessors_String(
        const UObject* InWorldContextObject,
        FName InKey,
        const FCk_Delegate_GameSettings_ExternalGetter_String& InGetter,
        const FCk_Delegate_GameSettings_ApplyHandler_String& InSetter)
    -> bool
{
    auto* Subsystem = DoGet_Subsystem(InWorldContextObject);

    if (ck::Is_NOT_Valid(Subsystem))
    { return false; }

    return Subsystem->Request_RegisterExternalAccessors_String(InKey, InGetter, InSetter);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_GameSettings_UE::
    Request_BeginPendingChanges(
        const UObject* InWorldContextObject)
    -> bool
{
    auto* Subsystem = DoGet_Subsystem(InWorldContextObject);

    if (ck::Is_NOT_Valid(Subsystem))
    { return false; }

    return Subsystem->Request_BeginPendingChanges();
}

auto
    UCk_Utils_GameSettings_UE::
    Request_ApplyPendingChanges(
        const UObject* InWorldContextObject)
    -> int32
{
    auto* Subsystem = DoGet_Subsystem(InWorldContextObject);

    if (ck::Is_NOT_Valid(Subsystem))
    { return 0; }

    return Subsystem->Request_ApplyPendingChanges();
}

auto
    UCk_Utils_GameSettings_UE::
    Request_RevertPendingChanges(
        const UObject* InWorldContextObject)
    -> int32
{
    auto* Subsystem = DoGet_Subsystem(InWorldContextObject);

    if (ck::Is_NOT_Valid(Subsystem))
    { return 0; }

    return Subsystem->Request_RevertPendingChanges();
}

auto
    UCk_Utils_GameSettings_UE::
    Get_HasPendingChanges(
        const UObject* InWorldContextObject)
    -> bool
{
    auto* Subsystem = DoGet_Subsystem(InWorldContextObject);

    if (ck::Is_NOT_Valid(Subsystem))
    { return false; }

    return Subsystem->Get_HasPendingChanges();
}

auto
    UCk_Utils_GameSettings_UE::
    Get_HasUnappliedChange(
        const UObject* InWorldContextObject,
        FName InKey)
    -> bool
{
    auto* Subsystem = DoGet_Subsystem(InWorldContextObject);

    if (ck::Is_NOT_Valid(Subsystem))
    { return false; }

    return Subsystem->Get_HasUnappliedChange(InKey);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_GameSettings_UE::
    Request_FlushStorage(
        const UObject* InWorldContextObject)
    -> void
{
    auto* Subsystem = DoGet_Subsystem(InWorldContextObject);

    if (ck::Is_NOT_Valid(Subsystem))
    { return; }

    Subsystem->Request_FlushStorage();
}

auto
    UCk_Utils_GameSettings_UE::
    Request_ReloadFromStorage(
        const UObject* InWorldContextObject)
    -> int32
{
    auto* Subsystem = DoGet_Subsystem(InWorldContextObject);

    if (ck::Is_NOT_Valid(Subsystem))
    { return 0; }

    return Subsystem->Request_ReloadFromStorage();
}

auto
    UCk_Utils_GameSettings_UE::
    Get_StorageProvider(
        const UObject* InWorldContextObject)
    -> UCk_GameSettings_StorageProvider_UE*
{
    auto* Subsystem = DoGet_Subsystem(InWorldContextObject);

    if (ck::Is_NOT_Valid(Subsystem))
    { return nullptr; }

    return Subsystem->Get_StorageProvider();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_GameSettings_UE::
    DoGet_Subsystem(
        const UObject* InWorldContextObject)
    -> UCk_GameSettings_Subsystem_UE*
{
    const auto ContextIsValid = ck::IsValid(InWorldContextObject);
    CK_ENSURE_IF_NOT(ContextIsValid, TEXT("Invalid WorldContextObject, cannot resolve the GameSettings Subsystem"))
    {}
    if (NOT ContextIsValid)
    { return nullptr; }

    const auto* World = InWorldContextObject->GetWorld();
    const auto WorldIsValid = ck::IsValid(World);
    CK_ENSURE_IF_NOT(WorldIsValid, TEXT("WorldContextObject has no UWorld, cannot resolve the GameSettings Subsystem"))
    {}
    if (NOT WorldIsValid)
    { return nullptr; }

    const auto* GameInstance = World->GetGameInstance();
    const auto GameInstanceIsValid = ck::IsValid(GameInstance);
    CK_ENSURE_IF_NOT(GameInstanceIsValid, TEXT("World has no GameInstance, cannot resolve the GameSettings Subsystem"))
    {}
    if (NOT GameInstanceIsValid)
    { return nullptr; }

    auto* Subsystem = GameInstance->GetSubsystem<UCk_GameSettings_Subsystem_UE>();
    const auto SubsystemIsValid = ck::IsValid(Subsystem);
    CK_ENSURE_IF_NOT(SubsystemIsValid, TEXT("GameSettings Subsystem not found on the GameInstance"))
    {}
    if (NOT SubsystemIsValid)
    { return nullptr; }

    return Subsystem;
}

// --------------------------------------------------------------------------------------------------------------------
