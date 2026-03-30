// Copyright 2025 CkFoundation. All Rights Reserved.

#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkUI/Extension/CkUI_Extension_Types.h"

#include <Subsystems/LocalPlayerSubsystem.h>
#include <Blueprint/UserWidget.h>

#include "CkUI_Extension_Subsystem.generated.h"


// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_DynamicDelegate_ExtensionPoint_OnExtend,
    ECk_UI_ExtensionAction, Action,
    const FCk_UI_ExtensionRequest&, Request);

// --------------------------------------------------------------------------------------------------------------------

/**
 * Per-player subsystem that manages UI extension points.
 *
 * Extension points define slots in the UI where widgets can be dynamically added.
 * Extensions are widget classes that get placed into extension points by tag matching.
 */
UCLASS(DisplayName = "CkSubsystem_UI_Extension")
class CKUI_API UCk_UI_Extension_Subsystem_UE : public ULocalPlayerSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_UI_Extension_Subsystem_UE);

    // ----------------------------------------------------------------------------------------------------------------
    // Subsystem Lifecycle
    // ----------------------------------------------------------------------------------------------------------------

public:
    virtual auto Initialize(FSubsystemCollectionBase& Collection) -> void override;
    virtual auto Deinitialize() -> void override;

    // ----------------------------------------------------------------------------------------------------------------
    // Extension Point Registration
    // ----------------------------------------------------------------------------------------------------------------

public:
    auto RegisterExtensionPoint(
        FGameplayTag InExtensionPointTag,
        ECk_UI_ExtensionPointMatch InMatchType,
        FCk_Delegate_ExtensionPoint_OnExtend InCallback) -> FCk_UI_ExtensionPointHandle;

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Extension",
        DisplayName = "[Ck][UI] Unregister Extension Point")
    void UnregisterExtensionPoint(const FCk_UI_ExtensionPointHandle& InHandle);

    // ----------------------------------------------------------------------------------------------------------------
    // Extension Registration
    // ----------------------------------------------------------------------------------------------------------------

public:
    auto RegisterExtension(
        FGameplayTag InExtensionPointTag,
        TSubclassOf<UUserWidget> InWidgetClass,
        int32 InPriority = INDEX_NONE) -> FCk_UI_ExtensionHandle;

    auto RegisterExtension(
        FGameplayTag InExtensionPointTag,
        UUserWidget* InWidgetInstance,
        int32 InPriority = INDEX_NONE) -> FCk_UI_ExtensionHandle;

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Extension",
        DisplayName = "[Ck][UI] Unregister Extension")
    void UnregisterExtension(const FCk_UI_ExtensionHandle& InHandle);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Extension",
        DisplayName = "[Ck][UI] Try Unregister Extension (By Widget)")
    bool TryUnregisterExtensionByWidget(UUserWidget* InWidget);

    UFUNCTION(BlueprintPure, Category = "Ck|UI|Extension",
        DisplayName = "[Ck][UI] Has Extensions At Point",
        meta = (Categories = "UI.ExtensionPoint"))
    bool HasExtensionsAtPoint(FGameplayTag InExtensionPointTag) const;

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Extension",
        DisplayName = "[Ck][UI] Clear Extensions At Point",
        meta = (Categories = "UI.ExtensionPoint"))
    void ClearExtensionsAtPoint(FGameplayTag InExtensionPointTag);

    // ----------------------------------------------------------------------------------------------------------------
    // Widget ↔ Extension Mapping
    // ----------------------------------------------------------------------------------------------------------------

public:
    auto NotifyWidgetCreatedForExtension(UUserWidget* InWidget, const FCk_UI_ExtensionHandle& InHandle) -> void;
    auto NotifyWidgetRemovedForExtension(UUserWidget* InWidget) -> void;

    // ----------------------------------------------------------------------------------------------------------------
    // Blueprint Registration Functions
    // ----------------------------------------------------------------------------------------------------------------

protected:
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Extension",
        DisplayName = "[Ck][UI] Register Extension Point",
        meta = (Categories = "UI.ExtensionPoint"))
    FCk_UI_ExtensionPointHandle DoRegisterExtensionPoint(
        FGameplayTag InExtensionPointTag,
        ECk_UI_ExtensionPointMatch InMatchType,
        FCk_DynamicDelegate_ExtensionPoint_OnExtend InCallback);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Extension",
        DisplayName = "[Ck][UI] Register Extension",
        meta = (Categories = "UI.ExtensionPoint"))
    FCk_UI_ExtensionHandle DoRegisterExtension(
        FGameplayTag InExtensionPointTag,
        TSubclassOf<UUserWidget> InWidgetClass,
        int32 InPriority = -1);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Extension",
        DisplayName = "[Ck][UI] Register Extension (Instance)",
        meta = (Categories = "UI.ExtensionPoint"))
    FCk_UI_ExtensionHandle DoRegisterExtension_Instance(
        FGameplayTag InExtensionPointTag,
        UUserWidget* InWidgetInstance,
        int32 InPriority = -1);

    // ----------------------------------------------------------------------------------------------------------------
    // Internal
    // ----------------------------------------------------------------------------------------------------------------

private:
    auto DoNotifyExtensionPointOfExtensions(
const TSharedPtr<FCk_UI_ExtensionPoint>& InExtensionPoint) -> void;
    auto DoNotifyExtensionPointsOfExtension(ECk_UI_ExtensionAction InAction,
 const TSharedPtr<FCk_UI_Extension>& InExtension) -> void;
    auto DoCreateExtensionRequest(const TSharedPtr<FCk_UI_Extension>& InExtension) -> FCk_UI_ExtensionRequest;

    // ----------------------------------------------------------------------------------------------------------------
    // Properties
    // ----------------------------------------------------------------------------------------------------------------

private:
    using FExtensionPointList = TArray<TSharedPtr<FCk_UI_ExtensionPoint>>;
    TMap<FGameplayTag, FExtensionPointList> _ExtensionPointMap;

    using FExtensionList = TArray<TSharedPtr<FCk_UI_Extension>>;
    TMap<FGameplayTag, FExtensionList> _ExtensionMap;

    TMap<TWeakObjectPtr<UUserWidget>, FCk_UI_ExtensionHandle> _WidgetToExtensionMap;
};

// --------------------------------------------------------------------------------------------------------------------