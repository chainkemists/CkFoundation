// Copyright 2025 CkFoundation. All Rights Reserved.

#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkUI/Types/CkUI_Types.h"

#include <Blueprint/UserWidget.h>
#include <Blueprint/WidgetTree.h>
#include <Components/NamedSlotInterface.h>
#include <Kismet/BlueprintFunctionLibrary.h>
#include <CommonInputTypeEnum.h>

#include "CkUI_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCommonActivatableWidget;
class UCk_UI_Subsystem_UE;
class UCk_UI_Layout_UE;
class UCk_UI_LayerStack_UE;
class APlayerController;
class ULocalPlayer;
class UWidget;
class UUserWidget;
class UNamedSlot;
class UPanelSlot;

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_UI_Widget_ViewportOperation : uint8
{
    DoNothing,
    AddToViewport,
    RemoveFromViewport
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_UI_Widget_ViewportOperation);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_UI_NamedSlot_SearchRecursive : uint8
{
    NonRecursive,
    Recursive
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_UI_NamedSlot_SearchRecursive);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_UI_NamedSlot_EnsureSlotIsFound : uint8
{
    EnsureSlotIsFound,
    AllowSlotNotFound
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_UI_NamedSlot_EnsureSlotIsFound);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_UI_NewMouseVisibility : uint8
{
    DontChange,
    Show,
    Hide,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_UI_NewMouseVisibility);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_UI_NewInputMode : uint8
{
    DontChange,
    GameOnly,
    UIOnly,
    GameAndUI
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_UI_NewInputMode);

// --------------------------------------------------------------------------------------------------------------------

/**
 * Static utility functions for the CkUI system.
 */
UCLASS(NotBlueprintable)
class CKUI_API UCk_Utils_UI_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_UI_UE);

    // ----------------------------------------------------------------------------------------------------------------
    // Subsystem & Layout Access
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintPure, Category = "Ck|UI",
        DisplayName = "[Ck][UI] Get Subsystem",
        meta = (DefaultToSelf = "InPlayerController"))
    static UCk_UI_Subsystem_UE* Get_UISubsystem(const APlayerController* InPlayerController);

    static UCk_UI_Subsystem_UE* Get_UISubsystem(const ULocalPlayer* InLocalPlayer);

    UFUNCTION(BlueprintPure, Category = "Ck|UI",
        DisplayName = "[Ck][UI] Get Layout",
        meta = (DefaultToSelf = "InPlayerController"))
    static UCk_UI_Layout_UE* Get_Layout(const APlayerController* InPlayerController);

    UFUNCTION(BlueprintPure, Category = "Ck|UI",
        DisplayName = "[Ck][UI] Get Layer",
        meta = (DefaultToSelf = "InPlayerController", Categories = "UI.Layer"))
    static UCk_UI_LayerStack_UE* Get_Layer(const APlayerController* InPlayerController, FGameplayTag InLayerTag);

    // ----------------------------------------------------------------------------------------------------------------
    // Layer Operations
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|UI",
        DisplayName = "[Ck][UI] Push Widget To Layer",
        meta = (DefaultToSelf = "InPlayerController", Categories = "UI.Layer",
            DeterminesOutputType = "InWidgetClass", DynamicOutputParam = "ReturnValue"))
    static UCommonActivatableWidget* PushWidgetToLayer(
        const APlayerController* InPlayerController,
        FGameplayTag InLayerTag,
        TSubclassOf<UCommonActivatableWidget> InWidgetClass);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI",
        DisplayName = "[Ck][UI] Push Widget To Layer (Soft)",
        meta = (DefaultToSelf = "InPlayerController", Categories = "UI.Layer"))
    static void PushWidgetToLayer_Soft(
        const APlayerController* InPlayerController,
        FGameplayTag InLayerTag,
        TSoftClassPtr<UCommonActivatableWidget> InWidgetClass,
        FCk_Delegate_UI_OnWidgetReady InOnWidgetReady);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI",
        DisplayName = "[Ck][UI] Push Widget Instance To Layer",
        meta = (DefaultToSelf = "InPlayerController", Categories = "UI.Layer"))
    static UCommonActivatableWidget* PushWidgetInstanceToLayer(
        const APlayerController* InPlayerController,
        FGameplayTag InLayerTag,
        UCommonActivatableWidget* InWidget);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI",
        DisplayName = "[Ck][UI] Pop Widget From Layer",
        meta = (DefaultToSelf = "InPlayerController", Categories = "UI.Layer"))
    static UCommonActivatableWidget* PopWidgetFromLayer(
        const APlayerController* InPlayerController,
        FGameplayTag InLayerTag);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI",
        DisplayName = "[Ck][UI] Clear Layer",
        meta = (DefaultToSelf = "InPlayerController", Categories = "UI.Layer"))
    static void ClearLayer(
        const APlayerController* InPlayerController,
        FGameplayTag InLayerTag);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI",
        DisplayName = "[Ck][UI] Remove Widget",
        meta = (DefaultToSelf = "InPlayerController"))
    static bool RemoveWidget(
        const APlayerController* InPlayerController,
        UCommonActivatableWidget* InWidget);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI",
        DisplayName = "[Ck][UI] Remove Widget (Self)",
        meta = (DefaultToSelf = "InWidget"))
    static bool RemoveWidgetSelf(UCommonActivatableWidget* InWidget);

    // ----------------------------------------------------------------------------------------------------------------
    // Named Slot Operations
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|NamedSlot",
        DisplayName = "[Ck][UI] Find Named Slot")
    static UNamedSlot* FindNamedSlot(
        UUserWidget* InSourceWidget,
        FName InNamedSlotName,
        ECk_UI_NamedSlot_SearchRecursive InIsRecursive = ECk_UI_NamedSlot_SearchRecursive::NonRecursive);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|NamedSlot",
        DisplayName = "[Ck][UI] Is Named Slot Occupied")
    static bool IsNamedSlotOccupied(UNamedSlot* InNamedSlot);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|NamedSlot",
        DisplayName = "[Ck][UI] Insert Widget To Named Slot")
    static UPanelSlot* InsertWidgetToNamedSlot(
        UNamedSlot* InNamedSlot,
        UUserWidget* InInsertedWidget);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|NamedSlot",
        DisplayName = "[Ck][UI] Find Named Slot And Insert Widget",
        meta = (AdvancedDisplay = "InEnsureSlotIsFound"))
    static UPanelSlot* FindNamedSlotAndInsertWidget(
        UUserWidget* InSourceWidget,
        UUserWidget* InInsertedWidget,
        FName InNamedSlotName,
        ECk_UI_NamedSlot_SearchRecursive InIsRecursive = ECk_UI_NamedSlot_SearchRecursive::NonRecursive,
        ECk_UI_NamedSlot_EnsureSlotIsFound InEnsureSlotIsFound = ECk_UI_NamedSlot_EnsureSlotIsFound::EnsureSlotIsFound);

    // ----------------------------------------------------------------------------------------------------------------
    // Focus Management
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Focus",
        DisplayName = "[Ck][UI] Get Currently Focused Widget")
    static UWidget* Get_CurrentlyFocusedWidget(int32 InUserIndex = 0);

    UFUNCTION(BlueprintCallable,
        DisplayName = "[Ck] Set Input Mode And Mouse Visibility",
        Category = "Ck|Utils|UI")
    static void SetInputModeAndMouseVisibility(
        APlayerController* InPlayerController,
        ECk_UI_NewInputMode InNewInputMode,
        ECk_UI_NewMouseVisibility InNewMouseVisibility,
        bool InResetCursorToCenter);

    // ----------------------------------------------------------------------------------------------------------------
    // Context Injection (Routes through Subsystem)
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Context",
        DisplayName = "[Ck][UI] Inject Context",
        meta = (DefaultToSelf = "InWidget"))
    static void InjectContext(UUserWidget* InWidget, const FCk_UI_Context& InContext);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Context",
        DisplayName = "[Ck][UI] Inject Entity Context",
        meta = (DefaultToSelf = "InWidget"))
    static void InjectEntityContext(UUserWidget* InWidget, FCk_Handle InEntity);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Context",
        DisplayName = "[Ck][UI] Inject Actor Context",
        meta = (DefaultToSelf = "InWidget"))
    static void InjectActorContext(UUserWidget* InWidget, AActor* InActor);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Context",
        DisplayName = "[Ck][UI] Clear Context",
        meta = (DefaultToSelf = "InWidget"))
    static void ClearContext(UUserWidget* InWidget);

    // ----------------------------------------------------------------------------------------------------------------
    // Context Query (From Subsystem Registry)
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintPure, Category = "Ck|UI|Context",
        DisplayName = "[Ck][UI] Get Context For Widget",
        meta = (DefaultToSelf = "InWidget"))
    static FCk_UI_Context Get_ContextForWidget(const UUserWidget* InWidget);

    UFUNCTION(BlueprintPure, Category = "Ck|UI|Context",
        DisplayName = "[Ck][UI] Has Valid Context",
        meta = (DefaultToSelf = "InWidget"))
    static bool Has_ValidContextForWidget(const UUserWidget* InWidget);

    UFUNCTION(BlueprintPure, Category = "Ck|UI|Context",
        DisplayName = "[Ck][UI] Get Context Entity",
        meta = (DefaultToSelf = "InWidget"))
    static FCk_Handle Get_ContextEntity(const UUserWidget* InWidget);

    UFUNCTION(BlueprintPure, Category = "Ck|UI|Context",
        DisplayName = "[Ck][UI] Get Context Actor",
        meta = (DefaultToSelf = "InWidget"))
    static AActor* Get_ContextActor(const UUserWidget* InWidget);

    UFUNCTION(BlueprintPure, Category = "Ck|UI|Context",
        DisplayName = "[Ck][UI] Get Context Payload",
        meta = (DefaultToSelf = "InWidget"))
    static UObject* Get_ContextPayload(const UUserWidget* InWidget);

    // ----------------------------------------------------------------------------------------------------------------
    // Context Creation Helpers
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintPure, Category = "Ck|UI|Context",
        DisplayName = "[Ck][UI] Make Context (Entity)")
    static FCk_UI_Context MakeContext_Entity(FCk_Handle InEntity);

    UFUNCTION(BlueprintPure, Category = "Ck|UI|Context",
        DisplayName = "[Ck][UI] Make Context (Actor)")
    static FCk_UI_Context MakeContext_Actor(AActor* InActor);

    UFUNCTION(BlueprintPure, Category = "Ck|UI|Context",
        DisplayName = "[Ck][UI] Make Context (Object)")
    static FCk_UI_Context MakeContext_Object(UObject* InObject);

    UFUNCTION(BlueprintPure, Category = "Ck|UI|Context",
        DisplayName = "[Ck][UI] Make Context (Entity + Actor)")
    static FCk_UI_Context MakeContext_EntityAndActor(FCk_Handle InEntity, AActor* InActor);

    // ----------------------------------------------------------------------------------------------------------------
    // Input Operations
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintPure, Category = "Ck|UI|Input",
        DisplayName = "[Ck][UI] Get Effective Input Mode",
        meta = (DefaultToSelf = "InPlayerController"))
    static ECk_UI_InputMode Get_EffectiveInputMode(const APlayerController* InPlayerController);

    // --------------------------------------------------------------------------------------------------------------------
    // Input Suspension
    // --------------------------------------------------------------------------------------------------------------------

public:
    /**
     * Suspends all input for a player using CommonUI's input filtering.
     * Returns a token that must be passed to ResumeInput.
     *
     * This is the canonical location for input suspension - operates directly
     * on CommonInputSubsystem without going through the UI Subsystem.
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Input",
        DisplayName = "[Ck][UI] Suspend Input",
        meta = (DefaultToSelf = "InPlayerController"))
    static FName SuspendInput(const APlayerController* InPlayerController, FName InReason);

    /** Overload accepting LocalPlayer directly. */
    static FName SuspendInput(const ULocalPlayer* InLocalPlayer, FName InReason);

    /** Resumes input for a player using a previously obtained suspension token. */
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Input",
        DisplayName = "[Ck][UI] Resume Input",
        meta = (DefaultToSelf = "InPlayerController"))
    static void ResumeInput(const APlayerController* InPlayerController, FName InSuspendToken);

    /** Overload accepting LocalPlayer directly. */
    static void ResumeInput(const ULocalPlayer* InLocalPlayer, FName InSuspendToken);

    // ----------------------------------------------------------------------------------------------------------------
    // Input Type Queries
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintPure, Category = "Ck|UI|Input",
        DisplayName = "[Ck][UI] Get Owning Player Input Type",
        meta = (DefaultToSelf = "InWidget"))
    static ECommonInputType Get_OwningPlayerInputType(const UUserWidget* InWidget);

    UFUNCTION(BlueprintPure, Category = "Ck|UI|Input",
        DisplayName = "[Ck][UI] Is Using Gamepad",
        meta = (DefaultToSelf = "InWidget"))
    static bool IsOwningPlayerUsingGamepad(const UUserWidget* InWidget);

    UFUNCTION(BlueprintPure, Category = "Ck|UI|Input",
        DisplayName = "[Ck][UI] Is Using Touch",
        meta = (DefaultToSelf = "InWidget"))
    static bool IsOwningPlayerUsingTouch(const UUserWidget* InWidget);

    UFUNCTION(BlueprintPure, Category = "Ck|UI|Input",
        DisplayName = "[Ck][UI] Is Using Mouse And Keyboard",
        meta = (DefaultToSelf = "InWidget"))
    static bool IsOwningPlayerUsingMouseAndKeyboard(const UUserWidget* InWidget);

    // ----------------------------------------------------------------------------------------------------------------
    // Widget Iteration (C++ Only)
    // ----------------------------------------------------------------------------------------------------------------
public:
    template <typename T_Predicate>
    static auto ForEachWidgetAndChildren_IncludingUserWidgets(
        UUserWidget* InUserWidget,
        T_Predicate InPred) -> void;

    template <typename T_Predicate>
    static auto ForEachWidgetAndChildren_IncludingUserWidgets(
        UWidget* InWidget,
        T_Predicate InPred) -> void;

private:
    /** Counter for generating unique suspension tokens. */
    static int32 InputSuspensionCounter;
};

// --------------------------------------------------------------------------------------------------------------------

template <typename T_Predicate>
auto
    UCk_Utils_UI_UE::
    ForEachWidgetAndChildren_IncludingUserWidgets(
        UUserWidget* InUserWidget,
        T_Predicate InPred)
    -> void
{
    if (ck::Is_NOT_Valid(InUserWidget))
    { return; }

    if (auto* RootWidget = InUserWidget->GetRootWidget();
        ck::IsValid(RootWidget))
    {
        ForEachWidgetAndChildren_IncludingUserWidgets(RootWidget, InPred);
    }
}

template <typename Predicate>
auto
    UCk_Utils_UI_UE::
    ForEachWidgetAndChildren_IncludingUserWidgets(
        UWidget* InWidget,
        Predicate InPred)
    -> void
{
    if (ck::Is_NOT_Valid(InWidget))
    { return; }

    if (InPred(InWidget))
    { return; }

    if (const auto* UserWidget = Cast<UUserWidget>(InWidget);
        ck::IsValid(UserWidget))
    {
        if (const auto* WidgetTree = UserWidget->WidgetTree.Get();
            ck::IsValid(WidgetTree))
        {
            ForEachWidgetAndChildren_IncludingUserWidgets(WidgetTree->RootWidget, InPred);
        }
    }

    if (const auto* NamedSlotHost = Cast<INamedSlotInterface>(InWidget);
        ck::IsValid(NamedSlotHost, ck::IsValid_Policy_NullptrOnly{}))
    {
        TArray<FName> SlotNames;
        NamedSlotHost->GetSlotNames(SlotNames);

        for (const auto& SlotName : SlotNames)
        {
            auto* SlotContent = NamedSlotHost->GetContentForSlot(SlotName);

            if (ck::Is_NOT_Valid(SlotContent))
            { continue; }

            ForEachWidgetAndChildren_IncludingUserWidgets(SlotContent, InPred);
        }
    }

    if (const auto* PanelParent = Cast<UPanelWidget>(InWidget);
        ck::IsValid(PanelParent, ck::IsValid_Policy_NullptrOnly{}))
    {
        for (auto ChildIndex = 0; ChildIndex < PanelParent->GetChildrenCount(); ++ChildIndex)
        {
            auto* ChildWidget = PanelParent->GetChildAt(ChildIndex);

            if (ck::Is_NOT_Valid(ChildWidget))
            { continue; }

            ForEachWidgetAndChildren_IncludingUserWidgets(ChildWidget, InPred);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------