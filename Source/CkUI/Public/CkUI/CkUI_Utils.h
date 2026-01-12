// Copyright 2025 CkFoundation. All Rights Reserved.

#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"
#include "CkCore/Format/CkFormat.h"

#include "CkUI/Types/CkUI_Types.h"

#include <Blueprint/UserWidget.h>
#include <Blueprint/WidgetTree.h>
#include <Components/NamedSlotInterface.h>
#include <Components/PanelWidget.h>
#include <CommonInputTypeEnum.h>
#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkUI_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class APlayerController;
class ULocalPlayer;
class UWidget;
class UUserWidget;
class UNamedSlot;
class UPanelSlot;

// --------------------------------------------------------------------------------------------------------------------

/**
 * Generic UI utility functions.
 *
 * Contains:
 * - Named slot operations
 * - Focus management
 * - Input mode/mouse visibility helpers
 * - Input suspension (CommonUI)
 * - Input type queries
 * - Widget iteration templates
 */
UCLASS(NotBlueprintable)
class CKUI_API UCk_Utils_UI_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_UI_UE);

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

    UFUNCTION(BlueprintCallable, Category = "Ck|UI",
        DisplayName = "[Ck][UI] Set Input Mode And Mouse Visibility")
    static void SetInputModeAndMouseVisibility(
        APlayerController* InPlayerController,
        ECk_UI_NewInputMode InNewInputMode,
        ECk_UI_NewMouseVisibility InNewMouseVisibility,
        bool InResetCursorToCenter);

    // ----------------------------------------------------------------------------------------------------------------
    // Input Suspension (CommonUI)
    // ----------------------------------------------------------------------------------------------------------------

public:
    /**
     * Suspends all input for a player using CommonUI's input filtering.
     * Returns a token that must be passed to ResumeInput.
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Input",
        DisplayName = "[Ck][UI] Suspend Input",
        meta = (DefaultToSelf = "InPlayerController"))
    static FName SuspendInput(const APlayerController* InPlayerController, FName InReason);

    static FName SuspendInput(const ULocalPlayer* InLocalPlayer, FName InReason);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Input",
        DisplayName = "[Ck][UI] Resume Input",
        meta = (DefaultToSelf = "InPlayerController"))
    static void ResumeInput(const APlayerController* InPlayerController, FName InSuspendToken);

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
    static void ForEachWidgetAndChildren_IncludingUserWidgets(
        UUserWidget* InUserWidget,
        T_Predicate InPred);

    template <typename T_Predicate>
    static void ForEachWidgetAndChildren_IncludingUserWidgets(
        UWidget* InWidget,
        T_Predicate InPred);

private:
    static int32 InputSuspensionCounter;
};

// --------------------------------------------------------------------------------------------------------------------
// Template Definitions
// --------------------------------------------------------------------------------------------------------------------

template <typename T_Predicate>
void UCk_Utils_UI_UE::ForEachWidgetAndChildren_IncludingUserWidgets(
    UUserWidget* InUserWidget,
    T_Predicate InPred)
{
    if (ck::Is_NOT_Valid(InUserWidget))
    { return; }

    if (auto* RootWidget = InUserWidget->GetRootWidget();
        ck::IsValid(RootWidget))
    {
        ForEachWidgetAndChildren_IncludingUserWidgets(RootWidget, InPred);
    }
}

template <typename T_Predicate>
void UCk_Utils_UI_UE::ForEachWidgetAndChildren_IncludingUserWidgets(
    UWidget* InWidget,
    T_Predicate InPred)
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