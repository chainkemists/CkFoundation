// Copyright 2025 CkFoundation. All Rights Reserved.

#include "CkUI/Subsystem/CkUI_Subsystem.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkUI/CkUI_Utils.h"
#include "CkUI/Layout/CkUI_Layout.h"
#include "CkUI/Layer/CkUI_LayerConfigAsset.h"
#include "CkUI/Interfaces/CkUI_Interfaces.h"
#include "CkUI/CustomWidgets/Watermark/CkWatermark_Widget.h"
#include "CkUI/ScreenFade/CkScreenFade_Widget.h"
#include "CkUI/Settings/CkUI_Settings.h"

#include <Blueprint/UserWidget.h>
#include <Blueprint/WidgetTree.h>
#include <Engine/AssetManager.h>
#include <Engine/StreamableManager.h>
#include <Engine/GameViewportClient.h>
#include <GameFramework/PlayerController.h>

// --------------------------------------------------------------------------------------------------------------------
// CVar for Watermark Display Policy
// --------------------------------------------------------------------------------------------------------------------

namespace ck_ui::cvar
{
    static int32 WatermarkDisplayPolicy = static_cast<int32>(ECk_Watermark_DisplayPolicy::Regular);

    static FAutoConsoleVariableRef CVar_WatermarkDisplayPolicy(
        TEXT("ck.UI.WatermarkDisplayPolicy"),
        WatermarkDisplayPolicy,
        TEXT("Set the Watermark Widget Display Policy (0=Hidden, 1=Regular, 2=Detailed)"),
        FConsoleVariableDelegate::CreateLambda([](IConsoleVariable* CVar)
        {
            const auto* GI = Cast<UGameInstance>(GEngine->GameViewport->GetGameInstance());

            if (ck::Is_NOT_Valid(GI))
            { return; }

            const auto* LocalPlayer = GI->FindLocalPlayerFromControllerId(0);

            if (ck::Is_NOT_Valid(LocalPlayer))
            { return; }

            auto* UISubsystem = LocalPlayer->GetSubsystem<UCk_UI_Subsystem_UE>();

            if (ck::Is_NOT_Valid(UISubsystem))
            { return; }

            UISubsystem->Request_UpdateWatermarkDisplayPolicy(
                static_cast<ECk_Watermark_DisplayPolicy>(WatermarkDisplayPolicy));
        }));
}

// --------------------------------------------------------------------------------------------------------------------
// Subsystem Lifecycle
// --------------------------------------------------------------------------------------------------------------------

void
    UCk_UI_Subsystem_UE::
    Initialize(
        FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    _SubsystemEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(_Registry);
}

void
    UCk_UI_Subsystem_UE::
    Deinitialize()
{
    DestroyPlayerLayout();
    DoDestroyWatermarkWidget();
    _WidgetContexts.Empty();

    if (ck::IsValid(_SubsystemEntity))
    {
        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(_SubsystemEntity);
        _SubsystemEntity = {};
    }

    Super::Deinitialize();
}

void
    UCk_UI_Subsystem_UE::
    PlayerControllerChanged(
        APlayerController* InNewPlayerController)
{
    Super::PlayerControllerChanged(InNewPlayerController);

    if (ck::Is_NOT_Valid(InNewPlayerController))
    { return; }

    DoCreateWatermarkWidget(InNewPlayerController);
}

// --------------------------------------------------------------------------------------------------------------------
// Player Management
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Subsystem_UE::
    CreatePlayerLayout(
        UCk_UI_LayerConfigAsset_UE* InConfigAsset)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InConfigAsset),
        TEXT("Cannot add player with invalid layer config asset"))
    { return; }

    if (Has_Layout())
    { return; }

    DoCreateLayout(InConfigAsset);
    DoLoadAndPushStartingWidgets(InConfigAsset);
}

auto
    UCk_UI_Subsystem_UE::
    DestroyPlayerLayout()
    -> void
{
    if (NOT Has_Layout())
    { return; }

    DoDestroyLayout();

    OnPlayerRemoved.Broadcast();
}

// --------------------------------------------------------------------------------------------------------------------
// Layout Access
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Subsystem_UE::
    TryGet_CurrentLayout() const
    -> UCk_UI_Layout_UE*
{
    return _Layout;
}

auto
    UCk_UI_Subsystem_UE::
    Has_Layout() const
    -> bool
{
    return ck::IsValid(_Layout);
}

// --------------------------------------------------------------------------------------------------------------------
// Layer Operations
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Subsystem_UE::
    PushWidgetToLayer(
        FGameplayTag InLayerTag,
        TSubclassOf<UCommonActivatableWidget> InWidgetClass)
    -> UCommonActivatableWidget*
{
    if (ck::Is_NOT_Valid(_Layout))
    { return nullptr; }

    return _Layout->PushWidgetToLayer(InLayerTag, InWidgetClass);
}

auto
    UCk_UI_Subsystem_UE::
    PushWidgetToLayer_Soft(
        FGameplayTag InLayerTag,
        TSoftClassPtr<UCommonActivatableWidget> InWidgetClass,
        FCk_Delegate_UI_OnWidgetReady InOnWidgetReady)
    -> void
{
    if (ck::Is_NOT_Valid(_Layout))
    {
        InOnWidgetReady.ExecuteIfBound(nullptr);
        return;
    }

    if (InWidgetClass.IsNull())
    {
        InOnWidgetReady.ExecuteIfBound(nullptr);
        return;
    }

    if (InWidgetClass.IsValid())
    {
        auto* Widget = PushWidgetToLayer(InLayerTag, InWidgetClass.Get());
        InOnWidgetReady.ExecuteIfBound(Widget);
        return;
    }

    const auto* LocalPlayer = GetLocalPlayer();
    const auto SuspendToken = UCk_Utils_UI_UE::SuspendInput(LocalPlayer, TEXT("AsyncWidgetLoad"));
    auto& StreamableManager = UAssetManager::GetStreamableManager();

    StreamableManager.RequestAsyncLoad(
        InWidgetClass.ToSoftObjectPath(),
        FStreamableDelegate::CreateWeakLambda(this, [this, InLayerTag, InWidgetClass, InOnWidgetReady, SuspendToken]()
        {
            UCk_Utils_UI_UE::ResumeInput(GetLocalPlayer(), SuspendToken);

            const auto LoadedClass = InWidgetClass.Get();

            if (ck::Is_NOT_Valid(LoadedClass))
            {
                InOnWidgetReady.ExecuteIfBound(nullptr);
                return;
            }

            auto* Widget = PushWidgetToLayer(InLayerTag, LoadedClass);
            InOnWidgetReady.ExecuteIfBound(Widget);
        }));
}

auto
    UCk_UI_Subsystem_UE::
    PushWidgetInstanceToLayer(
        FGameplayTag InLayerTag,
        UCommonActivatableWidget* InWidget)
    -> UCommonActivatableWidget*
{
    if (ck::Is_NOT_Valid(_Layout))
    { return nullptr; }

    return _Layout->PushWidgetInstanceToLayer(InLayerTag, InWidget);
}

auto
    UCk_UI_Subsystem_UE::
    PopWidgetFromLayer(
        FGameplayTag InLayerTag)
    -> UCommonActivatableWidget*
{
    if (ck::Is_NOT_Valid(_Layout))
    { return nullptr; }

    return _Layout->PopWidgetFromLayer(InLayerTag);
}

auto
    UCk_UI_Subsystem_UE::
    ClearLayer(
        FGameplayTag InLayerTag)
    -> void
{
    if (ck::Is_NOT_Valid(_Layout))
    { return; }

    _Layout->ClearLayer(InLayerTag);
}

auto
    UCk_UI_Subsystem_UE::
    RemoveWidget(
        UCommonActivatableWidget* InWidget)
    -> bool
{
    if (ck::Is_NOT_Valid(_Layout))
    { return false; }

    return _Layout->RemoveWidget(InWidget);
}

// --------------------------------------------------------------------------------------------------------------------
// Context Registry
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Subsystem_UE::
    InjectContextToWidget(
        UUserWidget* InWidget,
        const FCk_UI_Context& InContext)
    -> void
{
    if (ck::Is_NOT_Valid(InWidget))
    { return; }

    DoInjectContextRecursive(InWidget, InContext);
}

auto
    UCk_UI_Subsystem_UE::
    ClearContextFromWidget(
        UUserWidget* InWidget)
    -> void
{
    if (ck::Is_NOT_Valid(InWidget))
    { return; }

    DoClearContextRecursive(InWidget);
}

auto
    UCk_UI_Subsystem_UE::
    Get_ContextForWidget(
        const UUserWidget* InWidget) const
    -> FCk_UI_Context
{
    if (ck::Is_NOT_Valid(InWidget))
    { return {}; }

    const auto* FoundContext = _WidgetContexts.Find(const_cast<UUserWidget*>(InWidget));

    if (FoundContext == nullptr)
    { return {}; }

    return *FoundContext;
}

auto
    UCk_UI_Subsystem_UE::
    Has_ValidContextForWidget(
        const UUserWidget* InWidget) const
    -> bool
{
    return Get_ContextForWidget(InWidget).IsValid();
}

// --------------------------------------------------------------------------------------------------------------------
// Input Mode Query
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Subsystem_UE::
    Get_EffectiveInputMode() const
    -> ECk_UI_InputMode
{
    if (ck::Is_NOT_Valid(_Layout))
    { return ECk_UI_InputMode::GameOnly; }

    return _Layout->Get_EffectiveInputMode();
}

// --------------------------------------------------------------------------------------------------------------------
// Watermark
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Subsystem_UE::
    Request_UpdateWatermarkDisplayPolicy(
        ECk_Watermark_DisplayPolicy InDisplayPolicy) const
    -> void
{
    if (ck::Is_NOT_Valid(_WatermarkWidget))
    { return; }

    _WatermarkWidget->Request_SetDisplayPolicy(InDisplayPolicy);
}

// --------------------------------------------------------------------------------------------------------------------
// Screen Fade
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Subsystem_UE::
    Request_AddScreenFadeWidget(
        const FCk_ScreenFade_Params& InFadeParams,
        const APlayerController* InOwningPlayer,
        int32 InZOrder)
    -> void
{
    const auto ControllerID = DoGet_PlayerControllerID(InOwningPlayer);

    if (_FadeWidgetsForID.Contains(ControllerID))
    { DoRemoveScreenFadeWidget(InOwningPlayer, ControllerID); }

    auto OnFadeFinished = FCk_Delegate_OnScreenFadeFinished{};

    if (InFadeParams.Get_ToColor().A <= 0.0f)
    { OnFadeFinished.BindUObject(this, &ThisClass::DoRemoveScreenFadeWidget, ControllerID); }

    TSharedRef<SScreenFade_Widget> FadeWidget = SNew(SScreenFade_Widget)
        ._FadeParams(InFadeParams)
        ._OnFadeFinished(OnFadeFinished);

    if (auto* GameViewport = GetWorld()->GetGameViewport();
        ck::IsValid(GameViewport))
    {
        if (ck::IsValid(InOwningPlayer))
        { GameViewport->AddViewportWidgetForPlayer(InOwningPlayer->GetLocalPlayer(), FadeWidget, InZOrder); }
        else
        { GameViewport->AddViewportWidgetContent(FadeWidget, InZOrder + 10); }
    }

    _FadeWidgetsForID.Emplace(ControllerID, FadeWidget);
    FadeWidget->StartFade();
}

// --------------------------------------------------------------------------------------------------------------------
// Internal - Layout Management
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Subsystem_UE::
    DoCreateLayout(
        UCk_UI_LayerConfigAsset_UE* InConfigAsset)
    -> void
{
    if (ck::IsValid(_Layout))
    { return; }

    const auto* LocalPlayer = GetLocalPlayer();

    CK_ENSURE_IF_NOT(ck::IsValid(LocalPlayer),
        TEXT("UI Subsystem has no LocalPlayer during layout creation"))
    { return; }

    auto* PlayerController = LocalPlayer->GetPlayerController(GetWorld());

    CK_ENSURE_IF_NOT(ck::IsValid(PlayerController),
        TEXT("UI Subsystem LocalPlayer has no PlayerController during layout creation"))
    { return; }

    _Layout = CreateWidget<UCk_UI_Layout_UE>(PlayerController);

    CK_ENSURE_IF_NOT(ck::IsValid(_Layout),
        TEXT("Failed to create UI Layout widget"))
    { return; }

    _Layout->InitializeFromConfig(InConfigAsset);
    _Layout->AddToViewport();
    DoBindLayoutEvents();
}

auto
    UCk_UI_Subsystem_UE::
    DoDestroyLayout()
    -> void
{
    if (ck::Is_NOT_Valid(_Layout))
    { return; }

    DoUnbindLayoutEvents();

    _Layout->RemoveFromParent();
    _Layout = nullptr;
}

auto
    UCk_UI_Subsystem_UE::
    DoBindLayoutEvents()
    -> void
{
    if (ck::Is_NOT_Valid(_Layout))
    { return; }

    _Layout->OnWidgetPushed.AddUObject(this, &ThisClass::HandleLayoutWidgetPushed);
    _Layout->OnWidgetPopped.AddUObject(this, &ThisClass::HandleLayoutWidgetPopped);
    _Layout->OnLayerCleared.AddUObject(this, &ThisClass::HandleLayoutLayerCleared);
    _Layout->OnInputModeChanged.AddUObject(this, &ThisClass::HandleLayoutInputModeChanged);
}

auto
    UCk_UI_Subsystem_UE::
    DoUnbindLayoutEvents()
    -> void
{
    if (ck::Is_NOT_Valid(_Layout))
    { return; }

    _Layout->OnWidgetPushed.RemoveAll(this);
    _Layout->OnWidgetPopped.RemoveAll(this);
    _Layout->OnLayerCleared.RemoveAll(this);
    _Layout->OnInputModeChanged.RemoveAll(this);
}

auto
    UCk_UI_Subsystem_UE::
    DoLoadAndPushStartingWidgets(
        UCk_UI_LayerConfigAsset_UE* InConfigAsset)
    -> void
{
    if (ck::Is_NOT_Valid(InConfigAsset))
    {
        OnPlayerAdded.Broadcast();
        return;
    }

    auto PathsToLoad = TArray<FSoftObjectPath>{};

    ck::algo::ForEach(InConfigAsset->Get_LayerConfigs(), [&PathsToLoad](const FCk_UI_LayerConfig& InConfig)
    {
        ck::algo::ForEachIsValid(InConfig.Get_StartingWidgetClasses(), [&PathsToLoad](const TSoftClassPtr<UCommonActivatableWidget>& InWidgetClass)
        {
            PathsToLoad.Add(InWidgetClass.ToSoftObjectPath());
        });
    });

    if (PathsToLoad.IsEmpty())
    {
        OnPlayerAdded.Broadcast();
        return;
    }

    const auto* LocalPlayer = GetLocalPlayer();
    const auto SuspendToken = UCk_Utils_UI_UE::SuspendInput(LocalPlayer, TEXT("LoadStartingWidgets"));
    auto& StreamableManager = UAssetManager::GetStreamableManager();

    StreamableManager.RequestAsyncLoad(
        PathsToLoad,
        FStreamableDelegate::CreateWeakLambda(this, [this, InConfigAsset, SuspendToken]()
        {
            UCk_Utils_UI_UE::ResumeInput(GetLocalPlayer(), SuspendToken);

            if (ck::Is_NOT_Valid(_Layout))
            { return; }

            ck::algo::ForEach(InConfigAsset->Get_LayerConfigs(), [this](const FCk_UI_LayerConfig& InConfig)
            {
                ck::algo::ForEachIsValid(InConfig.Get_StartingWidgetClasses(), [this, &InConfig](const TSoftClassPtr<UCommonActivatableWidget>& InWidgetClass)
                {
                    _Layout->PushWidgetToLayer(InConfig.Get_LayerTag(), InWidgetClass.Get());
                });
            });

            OnPlayerAdded.Broadcast();
        }));
}

// --------------------------------------------------------------------------------------------------------------------
// Internal - Watermark
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Subsystem_UE::
    DoCreateWatermarkWidget(
        APlayerController* InPlayerController)
    -> void
{
    if (ck::IsValid(_WatermarkWidget))
    { return; }

    const auto& WatermarkWidgetClass = UCk_Utils_UI_Settings_UE::Get_WatermarkWidgetClass();

    if (ck::Is_NOT_Valid(WatermarkWidgetClass))
    { return; }

    if (ck::Is_NOT_Valid(InPlayerController))
    { return; }

    _WatermarkWidget = CreateWidget<UCk_Watermark_UserWidget_UE>(InPlayerController, WatermarkWidgetClass);

    CK_ENSURE_IF_NOT(ck::IsValid(_WatermarkWidget),
        TEXT("Failed to create the Watermark Widget"))
    { return; }

    _WatermarkWidget->Request_SetDisplayPolicy(
        static_cast<ECk_Watermark_DisplayPolicy>(ck_ui::cvar::WatermarkDisplayPolicy));

    _WatermarkWidget->AddToViewport(UCk_Utils_UI_Settings_UE::Get_WatermarkWidget_ZOrder());
}

auto
    UCk_UI_Subsystem_UE::
    DoDestroyWatermarkWidget()
    -> void
{
    if (ck::Is_NOT_Valid(_WatermarkWidget))
    { return; }

    _WatermarkWidget->RemoveFromParent();
    _WatermarkWidget = nullptr;
}

// --------------------------------------------------------------------------------------------------------------------
// Internal - Screen Fade
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Subsystem_UE::
    DoRemoveScreenFadeWidget(
        const APlayerController* InOwningPlayer,
        int32 InControllerID)
    -> void
{
    const auto FadeWidgetPtr = _FadeWidgetsForID.Find(InControllerID);

    if (FadeWidgetPtr == nullptr)
    { return; }

    const auto FadeWidget = FadeWidgetPtr->Pin();
    _FadeWidgetsForID.Remove(InControllerID);

    if (NOT FadeWidget.IsValid())
    { return; }

    auto* GameViewport = GetWorld()->GetGameViewport();

    if (ck::Is_NOT_Valid(GameViewport))
    { return; }

    if (ck::IsValid(InOwningPlayer))
    { GameViewport->RemoveViewportWidgetForPlayer(InOwningPlayer->GetLocalPlayer(), FadeWidget.ToSharedRef()); }
    else
    { GameViewport->RemoveViewportWidgetContent(FadeWidget.ToSharedRef()); }
}

auto
    UCk_UI_Subsystem_UE::
    DoRemoveScreenFadeWidget(
        int32 InControllerID)
    -> void
{
    DoRemoveScreenFadeWidget(DoGet_PlayerControllerFromID(InControllerID), InControllerID);
}

auto
    UCk_UI_Subsystem_UE::
    DoGet_PlayerControllerID(
        const APlayerController* InPlayerController) const
    -> int32
{
    if (ck::IsValid(InPlayerController))
    {
        if (const auto* LocalPlayer = InPlayerController->GetLocalPlayer();
            ck::IsValid(LocalPlayer))
        { return LocalPlayer->GetControllerId(); }
    }

    return InvalidPlayerControllerID;
}

auto
    UCk_UI_Subsystem_UE::
    DoGet_PlayerControllerFromID(
        int32 InControllerID) const
    -> APlayerController*
{
    if (InControllerID == InvalidPlayerControllerID)
    { return nullptr; }

    for (auto Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        if (auto* PlayerController = Iterator->Get();
            DoGet_PlayerControllerID(PlayerController) == InControllerID)
        { return PlayerController; }
    }

    return nullptr;
}

// --------------------------------------------------------------------------------------------------------------------
// Internal - Context Registry
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Subsystem_UE::
    DoRegisterWidget(
        UUserWidget* InWidget,
        const FCk_UI_Context& InContext)
    -> void
{
    if (ck::Is_NOT_Valid(InWidget))
    { return; }

    const auto IsNewEntry = NOT _WidgetContexts.Contains(InWidget);

    _WidgetContexts.Add(InWidget, InContext);

    if (IsNewEntry)
    { InWidget->OnNativeDestruct.AddUObject(this, &ThisClass::HandleWidgetDestroyed); }
}

auto
    UCk_UI_Subsystem_UE::
    DoUnregisterWidget(
        UUserWidget* InWidget)
    -> void
{
    if (ck::Is_NOT_Valid(InWidget))
    { return; }

    _WidgetContexts.Remove(InWidget);
    InWidget->OnNativeDestruct.RemoveAll(this);
}

auto
    UCk_UI_Subsystem_UE::
    DoInjectContextRecursive(
        UUserWidget* InWidget,
        const FCk_UI_Context& InContext)
    -> void
{
    if (ck::Is_NOT_Valid(InWidget))
    { return; }

    if (InWidget->Implements<UCk_UI_ContextReceiver>())
    {
        const auto* StoredContext = _WidgetContexts.Find(InWidget);

        if ([[maybe_unused]] const auto IsContextDifferent = (StoredContext == nullptr) || (*StoredContext != InContext))
        {
            DoRegisterWidget(InWidget, InContext);
            ICk_UI_ContextReceiver::Execute_OnContextInjected(InWidget, InContext);
        }
    }

    const auto& WidgetTree = InWidget->WidgetTree;

    if (ck::Is_NOT_Valid(WidgetTree))
    { return; }

    WidgetTree->ForEachWidget([this, &InContext](UWidget* TreeWidget)
    {
        if (ck::Is_NOT_Valid(TreeWidget))
        { return; }

        auto* UserWidget = Cast<UUserWidget>(TreeWidget);

        if (UserWidget == nullptr)
        { return; }

        if (UserWidget->Implements<UCk_UI_ContextReceiver>())
        {
            if (const auto ShouldInherit = ICk_UI_ContextReceiver::Execute_Get_ShouldInheritContextFromParent(UserWidget);
                NOT ShouldInherit)
            { return; }

            const auto* StoredContext = _WidgetContexts.Find(UserWidget);

            if ([[maybe_unused]] const auto IsContextDifferent = (StoredContext == nullptr) || (*StoredContext != InContext))
            {
                DoRegisterWidget(UserWidget, InContext);
                ICk_UI_ContextReceiver::Execute_OnContextInjected(UserWidget, InContext);
            }
        }
    });
}

auto
    UCk_UI_Subsystem_UE::
    DoClearContextRecursive(
        UUserWidget* InWidget)
    -> void
{
    if (ck::Is_NOT_Valid(InWidget))
    { return; }

    if (InWidget->Implements<UCk_UI_ContextReceiver>())
    {
        const auto* StoredContext = _WidgetContexts.Find(InWidget);

        if (StoredContext != nullptr && ck::IsValid(*StoredContext))
        {
            DoUnregisterWidget(InWidget);
            ICk_UI_ContextReceiver::Execute_OnContextCleared(InWidget);
        }
        else
        { DoUnregisterWidget(InWidget); }
    }

    const auto& WidgetTree = InWidget->WidgetTree;

    if (ck::Is_NOT_Valid(WidgetTree))
    { return; }

    WidgetTree->ForEachWidget([this](UWidget* TreeWidget)
    {
        if (ck::Is_NOT_Valid(TreeWidget))
        { return; }

        auto* UserWidget = Cast<UUserWidget>(TreeWidget);

        if (UserWidget == nullptr)
        { return; }

        if (UserWidget->Implements<UCk_UI_ContextReceiver>())
        {
            if (const auto ShouldInherit = ICk_UI_ContextReceiver::Execute_Get_ShouldInheritContextFromParent(UserWidget);
                NOT ShouldInherit)
            { return; }

            const auto* StoredContext = _WidgetContexts.Find(UserWidget);

            if (StoredContext != nullptr && ck::IsValid(*StoredContext))
            {
                DoUnregisterWidget(UserWidget);
                ICk_UI_ContextReceiver::Execute_OnContextCleared(UserWidget);
            }
            else
            { DoUnregisterWidget(UserWidget); }
        }
    });
}

void
    UCk_UI_Subsystem_UE::
    HandleWidgetDestroyed(
        UUserWidget* InWidget)
{
    _WidgetContexts.Remove(InWidget);
}

// --------------------------------------------------------------------------------------------------------------------
// Internal - Event Handlers
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Subsystem_UE::
    HandleLayoutWidgetPushed(
        FGameplayTag InLayerTag,
        UCommonActivatableWidget* InWidget) const
    -> void
{
    OnWidgetPushed.Broadcast(InWidget);
}

auto
    UCk_UI_Subsystem_UE::
    HandleLayoutWidgetPopped(
        FGameplayTag InLayerTag,
        UCommonActivatableWidget* InWidget) const
    -> void
{
    OnWidgetPopped.Broadcast(InWidget);
}

auto
    UCk_UI_Subsystem_UE::
    HandleLayoutLayerCleared(
        FGameplayTag InLayerTag) const
    -> void
{
    OnLayerCleared.Broadcast();
}

auto
    UCk_UI_Subsystem_UE::
    HandleLayoutInputModeChanged(
        ECk_UI_InputMode InNewMode) const
    -> void
{
    OnInputModeChanged.Broadcast(InNewMode);
}

// --------------------------------------------------------------------------------------------------------------------