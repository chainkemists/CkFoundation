#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkGameSettings/CkGameSettings_Common.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkGameSettings_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_GameSettings_Subsystem_UE;
class UCk_GameSettings_Collection_PDA;

// --------------------------------------------------------------------------------------------------------------------

/**
 * BP/AS surface of the GameSettings registry. Every function resolves the GameInstance's
 * UCk_GameSettings_Subsystem_UE from the world context and forwards to it.
 */
UCLASS(NotBlueprintable)
class CKGAMESETTINGS_API UCk_Utils_GameSettings_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_GameSettings_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Register Setting",
              meta = (WorldContext = "InWorldContextObject"))
    static bool
    Request_RegisterSetting(
        const UObject* InWorldContextObject,
        const FCk_GameSettings_SettingDefinition& InDefinition);

    /** Atomic: every definition is validated first; ONE invalid definition rejects the whole batch and registers nothing. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Register Settings",
              meta = (WorldContext = "InWorldContextObject"))
    static bool
    Request_RegisterSettings(
        const UObject* InWorldContextObject,
        const TArray<FCk_GameSettings_SettingDefinition>& InDefinitions);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Register Collection",
              meta = (WorldContext = "InWorldContextObject"))
    static bool
    Request_RegisterCollection(
        const UObject* InWorldContextObject,
        const UCk_GameSettings_Collection_PDA* InCollection);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GameSettings",
              DisplayName = "[Ck][GameSettings] Get Setting Value (Bool)",
              meta = (WorldContext = "InWorldContextObject"))
    static bool
    Get_SettingValue_Bool(
        const UObject* InWorldContextObject,
        FName InKey,
        bool InFallback = false);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GameSettings",
              DisplayName = "[Ck][GameSettings] Get Setting Value (Int32)",
              meta = (WorldContext = "InWorldContextObject"))
    static int32
    Get_SettingValue_Int32(
        const UObject* InWorldContextObject,
        FName InKey,
        int32 InFallback = 0);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GameSettings",
              DisplayName = "[Ck][GameSettings] Get Setting Value (Float)",
              meta = (WorldContext = "InWorldContextObject"))
    static float
    Get_SettingValue_Float(
        const UObject* InWorldContextObject,
        FName InKey,
        float InFallback = 0.0f);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GameSettings",
              DisplayName = "[Ck][GameSettings] Get Setting Value (String)",
              meta = (WorldContext = "InWorldContextObject"))
    static FString
    Get_SettingValue_String(
        const UObject* InWorldContextObject,
        FName InKey,
        const FString& InFallback = FString(TEXT("")));

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GameSettings",
              DisplayName = "[Ck][GameSettings] Get Is Setting Registered",
              meta = (WorldContext = "InWorldContextObject"))
    static bool
    Get_IsSettingRegistered(
        const UObject* InWorldContextObject,
        FName InKey);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GameSettings",
              DisplayName = "[Ck][GameSettings] Get Setting Definition",
              meta = (WorldContext = "InWorldContextObject"))
    static bool
    Get_SettingDefinition(
        const UObject* InWorldContextObject,
        FName InKey,
        FCk_GameSettings_SettingDefinition& OutDefinition);

    /** Keys in registration order. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GameSettings",
              DisplayName = "[Ck][GameSettings] Get All Setting Keys",
              meta = (WorldContext = "InWorldContextObject"))
    static TArray<FName>
    Get_AllSettingKeys(
        const UObject* InWorldContextObject);

    /** Keys (in registration order) whose CategoryTags match the query. An empty query matches every registered setting. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GameSettings",
              DisplayName = "[Ck][GameSettings] Get Setting Keys By Category",
              meta = (WorldContext = "InWorldContextObject"))
    static TArray<FName>
    Get_SettingKeysByCategory(
        const UObject* InWorldContextObject,
        const FGameplayTagQuery& InCategoryQuery);

public:
    /**
     * Validates (registered, type match, range/options), stores the value, and fires the change
     * delegate iff the value actually changed. Returns whether the value now holds — an
     * idempotent same-value set returns true without firing.
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Set Setting Value (Bool)",
              meta = (WorldContext = "InWorldContextObject"))
    static bool
    Request_SetSettingValue_Bool(
        const UObject* InWorldContextObject,
        const FCk_Request_GameSettings_SetValue_Bool& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Set Setting Value (Int32)",
              meta = (WorldContext = "InWorldContextObject"))
    static bool
    Request_SetSettingValue_Int32(
        const UObject* InWorldContextObject,
        const FCk_Request_GameSettings_SetValue_Int32& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Set Setting Value (Float)",
              meta = (WorldContext = "InWorldContextObject"))
    static bool
    Request_SetSettingValue_Float(
        const UObject* InWorldContextObject,
        const FCk_Request_GameSettings_SetValue_Float& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Set Setting Value (String)",
              meta = (WorldContext = "InWorldContextObject"))
    static bool
    Request_SetSettingValue_String(
        const UObject* InWorldContextObject,
        const FCk_Request_GameSettings_SetValue_String& InRequest);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Reset To Default",
              meta = (WorldContext = "InWorldContextObject"))
    static bool
    Request_ResetToDefault(
        const UObject* InWorldContextObject,
        const FCk_Request_GameSettings_ResetToDefault& InRequest);

    /** Returns how many settings actually changed back to their default. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Reset All To Defaults",
              meta = (WorldContext = "InWorldContextObject"))
    static int32
    Request_ResetAllToDefaults(
        const UObject* InWorldContextObject);

public:
    /** InKey == None binds to changes of EVERY setting (wildcard). Binding before the key is registered is legitimate. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GameSettings",
              DisplayName = "[Ck][GameSettings] Bind To OnSettingChanged",
              meta = (WorldContext = "InWorldContextObject"))
    static void
    BindTo_OnSettingChanged(
        const UObject* InWorldContextObject,
        FName InKey,
        const FCk_Delegate_GameSettings_OnSettingChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GameSettings",
              DisplayName = "[Ck][GameSettings] Unbind From OnSettingChanged",
              meta = (WorldContext = "InWorldContextObject"))
    static void
    UnbindFrom_OnSettingChanged(
        const UObject* InWorldContextObject,
        FName InKey,
        const FCk_Delegate_GameSettings_OnSettingChanged& InDelegate);

public:
    /**
     * Registers the apply target for a Handler-bound setting. Registering before the setting's
     * definition exists is legitimate; a handler arriving for a key with a pending stored value
     * applies that value immediately. Returns false (with ensure) only on a known type mismatch.
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Register Apply Handler (Bool)",
              meta = (WorldContext = "InWorldContextObject"))
    static bool
    Request_RegisterApplyHandler_Bool(
        const UObject* InWorldContextObject,
        FName InKey,
        const FCk_Delegate_GameSettings_ApplyHandler_Bool& InHandler);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Register Apply Handler (Int32)",
              meta = (WorldContext = "InWorldContextObject"))
    static bool
    Request_RegisterApplyHandler_Int32(
        const UObject* InWorldContextObject,
        FName InKey,
        const FCk_Delegate_GameSettings_ApplyHandler_Int32& InHandler);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Register Apply Handler (Float)",
              meta = (WorldContext = "InWorldContextObject"))
    static bool
    Request_RegisterApplyHandler_Float(
        const UObject* InWorldContextObject,
        FName InKey,
        const FCk_Delegate_GameSettings_ApplyHandler_Float& InHandler);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Register Apply Handler (String)",
              meta = (WorldContext = "InWorldContextObject"))
    static bool
    Request_RegisterApplyHandler_String(
        const UObject* InWorldContextObject,
        FName InKey,
        const FCk_Delegate_GameSettings_ApplyHandler_String& InHandler);

public:
    /**
     * Registers the read/write-through pair for an External-policy setting. External values are
     * NEVER stored in the provider: reads route through InGetter, writes through InSetter.
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Register External Accessors (Bool)",
              meta = (WorldContext = "InWorldContextObject"))
    static bool
    Request_RegisterExternalAccessors_Bool(
        const UObject* InWorldContextObject,
        FName InKey,
        const FCk_Delegate_GameSettings_ExternalGetter_Bool& InGetter,
        const FCk_Delegate_GameSettings_ApplyHandler_Bool& InSetter);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Register External Accessors (Int32)",
              meta = (WorldContext = "InWorldContextObject"))
    static bool
    Request_RegisterExternalAccessors_Int32(
        const UObject* InWorldContextObject,
        FName InKey,
        const FCk_Delegate_GameSettings_ExternalGetter_Int32& InGetter,
        const FCk_Delegate_GameSettings_ApplyHandler_Int32& InSetter);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Register External Accessors (Float)",
              meta = (WorldContext = "InWorldContextObject"))
    static bool
    Request_RegisterExternalAccessors_Float(
        const UObject* InWorldContextObject,
        FName InKey,
        const FCk_Delegate_GameSettings_ExternalGetter_Float& InGetter,
        const FCk_Delegate_GameSettings_ApplyHandler_Float& InSetter);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Register External Accessors (String)",
              meta = (WorldContext = "InWorldContextObject"))
    static bool
    Request_RegisterExternalAccessors_String(
        const UObject* InWorldContextObject,
        FName InKey,
        const FCk_Delegate_GameSettings_ExternalGetter_String& InGetter,
        const FCk_Delegate_GameSettings_ApplyHandler_String& InSetter);

public:
    /** Begins the single staged-changes session. While active, sets apply LIVE (preview) and record priors. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Begin Pending Changes",
              meta = (WorldContext = "InWorldContextObject"))
    static bool
    Request_BeginPendingChanges(
        const UObject* InWorldContextObject);

    /** Commits the session: previewed values stay, priors are discarded, storage is flushed. Returns how many settings were committed. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Apply Pending Changes",
              meta = (WorldContext = "InWorldContextObject"))
    static int32
    Request_ApplyPendingChanges(
        const UObject* InWorldContextObject);

    /** Reverts the session: every recorded prior is re-applied live. Returns how many settings changed back. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Revert Pending Changes",
              meta = (WorldContext = "InWorldContextObject"))
    static int32
    Request_RevertPendingChanges(
        const UObject* InWorldContextObject);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GameSettings",
              DisplayName = "[Ck][GameSettings] Get Has Pending Changes",
              meta = (WorldContext = "InWorldContextObject"))
    static bool
    Get_HasPendingChanges(
        const UObject* InWorldContextObject);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GameSettings",
              DisplayName = "[Ck][GameSettings] Get Has Unapplied Change",
              meta = (WorldContext = "InWorldContextObject"))
    static bool
    Get_HasUnappliedChange(
        const UObject* InWorldContextObject,
        FName InKey);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Flush Storage",
              meta = (WorldContext = "InWorldContextObject"))
    static void
    Request_FlushStorage(
        const UObject* InWorldContextObject);

    /**
     * Re-runs the boot merge against the storage provider: each stored value applies onto its
     * registered definition, or is retained as an orphan for a definition registered later.
     * Returns how many stored values were read.
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Reload From Storage",
              meta = (WorldContext = "InWorldContextObject"))
    static int32
    Request_ReloadFromStorage(
        const UObject* InWorldContextObject);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GameSettings",
              DisplayName = "[Ck][GameSettings] Get Storage Provider",
              meta = (WorldContext = "InWorldContextObject"))
    static UCk_GameSettings_StorageProvider_UE*
    Get_StorageProvider(
        const UObject* InWorldContextObject);

private:
    static auto
    DoGet_Subsystem(
        const UObject* InWorldContextObject) -> UCk_GameSettings_Subsystem_UE*;
};

// --------------------------------------------------------------------------------------------------------------------
