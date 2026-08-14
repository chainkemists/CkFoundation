// Copyright 2025 CkFoundation. All Rights Reserved.

#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"
#include "CkCore/Format/CkFormat.h"

#include "CkUICore/Subsystem/CkUI_Input_Subsystem.h"
#include "CkUICore/Types/CkUI_Types.h"

#include "Widgets/CommonActivatableWidgetContainer.h"
#include "CommonActivatableWidget.h"

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
struct FCk_Handle;

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
class CKUICORE_API UCk_Utils_UI_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_UI_UE);

    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|NamedSlot", BlueprintCosmetic,
        DisplayName = "[Ck][UI] Find Named Slot")
    static UNamedSlot* FindNamedSlot(
        UUserWidget* InSourceWidget,
        FName InNamedSlotName,
        ECk_UI_NamedSlot_SearchRecursive InIsRecursive = ECk_UI_NamedSlot_SearchRecursive::NonRecursive);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|NamedSlot", BlueprintCosmetic,
        DisplayName = "[Ck][UI] Is Named Slot Occupied")
    static bool IsNamedSlotOccupied(UNamedSlot* InNamedSlot);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|NamedSlot",
        DisplayName = "[Ck][UI] Insert Widget To Named Slot")
    static UPanelSlot* InsertWidgetToNamedSlot(
        UNamedSlot* InNamedSlot,
        UUserWidget* InInsertedWidget);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|NamedSlot", BlueprintCosmetic,
        DisplayName = "[Ck][UI] Find Named Slot And Insert Widget",
        meta = (AdvancedDisplay = "InEnsureSlotIsFound"))
    static UPanelSlot* FindNamedSlotAndInsertWidget(
        UUserWidget* InSourceWidget,
        UUserWidget* InInsertedWidget,
        FName InNamedSlotName,
        ECk_UI_NamedSlot_SearchRecursive InIsRecursive = ECk_UI_NamedSlot_SearchRecursive::NonRecursive,
        ECk_UI_NamedSlot_EnsureSlotIsFound InEnsureSlotIsFound = ECk_UI_NamedSlot_EnsureSlotIsFound::EnsureSlotIsFound);

    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Focus", BlueprintCosmetic,
        DisplayName = "[Ck][UI] Get Currently Focused Widget")
    static UWidget* Get_CurrentlyFocusedWidget(int32 InUserIndex = 0);

    // ----------------------------------------------------------------------------------------------------------------

public:
    /**
     * Installs a fresh Slate navigation config built from InConfig.
     *
     * FSlateApplication is PROCESS-global, not world- or player-scoped: whatever is set here persists
     * past PIE teardown into the editor's own UI. Callers that narrow navigation for gameplay are
     * expected to restore it (pass a default-constructed FCk_UI_NavigationConfig) on EndPlay.
     *
     * No-op when Slate is not initialized (dedicated server / early startup / commandlet).
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Navigation", BlueprintCosmetic,
        DisplayName = "[Ck][UI] Request Set Navigation Config")
    static void Request_SetNavigationConfig(
        const FCk_UI_NavigationConfig& InConfig);

    /** Reads back the live Slate navigation config. Returns defaults when Slate is not initialized. */
    UFUNCTION(BlueprintPure, Category = "Ck|UI|Navigation", BlueprintCosmetic,
        DisplayName = "[Ck][UI] Get Navigation Config")
    static FCk_UI_NavigationConfig Get_NavigationConfig();

    UFUNCTION(BlueprintCallable,
        DisplayName = "[Ck] Get Widgets Of Class In Hierarchy",
        Category = "Ck|Utils|UI",
        meta = (DeterminesOutputType = "InClass"))
    static TArray<UUserWidget*>
    Get_WidgetsOfClassInHierarchy(
        UUserWidget* InSourceWidget,
        TSubclassOf<UUserWidget> InClass);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI", BlueprintCosmetic,
        DisplayName = "[Ck][UI] Set Input Mode And Mouse Visibility")
    static void SetInputModeAndMouseVisibility(
        APlayerController* InPlayerController,
        ECk_UI_NewInputMode InNewInputMode,
        ECk_UI_NewMouseVisibility InNewMouseVisibility,
        bool InResetCursorToCenter);

    // ----------------------------------------------------------------------------------------------------------------

public:
    /**
     * Confines the mouse cursor to the screen bounds of InWidget.
     *
     * Slate owns the lock from here on: it recomputes the clip rect whenever the Widget's layout
     * bounds change and releases the cursor once the Widget leaves the screen. Callers do not need
     * per-frame upkeep, nor an unlock on teardown.
     *
     * The confinement is a single global platform state, so a subsequent call retargets the
     * existing lock rather than nesting under it.
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Cursor", BlueprintCosmetic,
        DisplayName = "[Ck][UI] Lock Cursor To Widget")
    static ECk_UI_CursorLock_Result
    Request_LockCursorToWidget(
        UWidget* InWidget);

    /**
     * Releases the active cursor confinement, including one the engine established rather than
     * Request_LockCursorToWidget.
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Cursor", BlueprintCosmetic,
        DisplayName = "[Ck][UI] Unlock Cursor")
    static void
    Request_UnlockCursor();

    // ----------------------------------------------------------------------------------------------------------------

public:
    /**
     * Suspends all input for a player using CommonUI's input filtering.
     * Returns a token that must be passed to ResumeInput.
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Input", BlueprintCosmetic,
        DisplayName = "[Ck][UI] Suspend Input",
        meta = (DefaultToSelf = "InPlayerController"))
    static FCk_Handle_InputSuspension SuspendInput(const APlayerController* InPlayerController, FName InReason);

    static FCk_Handle_InputSuspension SuspendInput(const ULocalPlayer* InLocalPlayer, FName InReason);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|Input", BlueprintCosmetic,
        DisplayName = "[Ck][UI] Resume Input",
        meta = (DefaultToSelf = "InPlayerController"))
    static void ResumeInput(const APlayerController* InPlayerController, FCk_Handle_InputSuspension& InSuspensionHandle);

    static void ResumeInput(const ULocalPlayer* InLocalPlayer, FCk_Handle_InputSuspension& InSuspensionHandle);

    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintPure, Category = "Ck|UI|Input", BlueprintCosmetic,
        DisplayName = "[Ck][UI] Get Owning Player Input Type",
        meta = (DefaultToSelf = "InWidget"))
    static ECommonInputType Get_OwningPlayerInputType(const UUserWidget* InWidget);

    UFUNCTION(BlueprintPure, Category = "Ck|UI|Input", BlueprintCosmetic,
        DisplayName = "[Ck][UI] Is Using Gamepad",
        meta = (DefaultToSelf = "InWidget"))
    static bool IsOwningPlayerUsingGamepad(const UUserWidget* InWidget);

    UFUNCTION(BlueprintPure, Category = "Ck|UI|Input", BlueprintCosmetic,
        DisplayName = "[Ck][UI] Is Using Touch",
        meta = (DefaultToSelf = "InWidget"))
    static bool IsOwningPlayerUsingTouch(const UUserWidget* InWidget);

    UFUNCTION(BlueprintPure, Category = "Ck|UI|Input", BlueprintCosmetic,
        DisplayName = "[Ck][UI] Is Using Mouse And Keyboard",
        meta = (DefaultToSelf = "InWidget"))
    static bool IsOwningPlayerUsingMouseAndKeyboard(const UUserWidget* InWidget);

    // ----------------------------------------------------------------------------------------------------------------

public:
    static auto
    PropagateContextToChildWidgets(
        UWidget* InRootWidget,
        const FCk_Handle& InContextEntity) -> void;

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

template <typename T_Predicate>
void UCk_Utils_UI_UE::ForEachWidgetAndChildren_IncludingUserWidgets(
    UUserWidget* InUserWidget,
    T_Predicate InPred)
{
    if (ck::Is_NOT_Valid(InUserWidget))
    { return; }

    if (InPred(InUserWidget) == ECk_UI_ForEachWidgetResult::SkipSubtree)
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

    if (InPred(InWidget) == ECk_UI_ForEachWidgetResult::SkipSubtree)
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

    if (const auto ContainerWidget = Cast<UCommonActivatableWidgetContainerBase>(InWidget);
        ck::IsValid(ContainerWidget))
    {
        for (auto ChildWidget : ContainerWidget->GetWidgetList())
        {
            if (ck::Is_NOT_Valid(ChildWidget))
            { continue; }

            ForEachWidgetAndChildren_IncludingUserWidgets(ChildWidget, InPred);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------