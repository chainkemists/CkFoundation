// Copyright 2025 CkFoundation. All Rights Reserved.

#include "CkUI/Layout/CkUI_Layout_Subsystem.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"
#include "CkUI/CkUI_Utils.h"
#include "CkUI/Extension/CkUI_Extension_Subsystem.h"
#include "CkUI/Layout/CkUI_PrimaryGameLayout.h"
#include "CkUI/Layout/CkUI_LayoutConfigAsset.h"
#include "CkUI/Layout/CkUI_LayerStack.h"

#include <Blueprint/UserWidget.h>
#include <Engine/AssetManager.h>
#include <Engine/StreamableManager.h>
#include <GameFramework/PlayerController.h>

// --------------------------------------------------------------------------------------------------------------------
// Subsystem Lifecycle
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Layout_Subsystem_UE::
    Initialize(
        FSubsystemCollectionBase& Collection)
    -> void
{
    Super::Initialize(Collection);
    Collection.InitializeDependency<UCk_UI_Extension_Subsystem_UE>();
}

auto
    UCk_UI_Layout_Subsystem_UE::
    Deinitialize()
    -> void
{
    DestroyLayout();
    Super::Deinitialize();
}

// --------------------------------------------------------------------------------------------------------------------
// Layout Management
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Layout_Subsystem_UE::
    CreateLayout(
        UCk_UI_LayoutConfigAsset_UE* InConfigAsset)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InConfigAsset),
        TEXT("Cannot create layout with invalid config asset"))
    { return; }

    if (Has_Layout())
    { return; }

    DoCreateLayout(InConfigAsset);
    DoLoadAndPushStartingWidgets(InConfigAsset);
    DoLoadAndRegisterHUDElements(InConfigAsset);
}

auto
    UCk_UI_Layout_Subsystem_UE::
    DestroyLayout()
    -> void
{
    if (NOT Has_Layout())
    { return; }

    DoUnregisterHUDElements();
    DoDestroyLayout();

    OnLayoutDestroyed.Broadcast();
    OnLayoutDestroyed_BP.Broadcast();
}

auto
    UCk_UI_Layout_Subsystem_UE::
    Get_Layout() const
    -> UCk_UI_PrimaryGameLayout_UE*
{
    return _Layout;
}

auto
    UCk_UI_Layout_Subsystem_UE::
    Has_Layout() const
    -> bool
{
    return ck::IsValid(_Layout);
}

// --------------------------------------------------------------------------------------------------------------------
// Layer Access
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Layout_Subsystem_UE::
    Get_Layer(
        FGameplayTag InLayerTag) const
    -> UCk_UI_LayerStack_UE*
{
    if (ck::Is_NOT_Valid(_Layout))
    { return nullptr; }

    return _Layout->Get_Layer(InLayerTag);
}

auto
    UCk_UI_Layout_Subsystem_UE::
    HasLayer(
        FGameplayTag InLayerTag) const
    -> bool
{
    if (ck::Is_NOT_Valid(_Layout))
    { return false; }

    return _Layout->HasLayer(InLayerTag);
}

// --------------------------------------------------------------------------------------------------------------------
// Widget Operations
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Layout_Subsystem_UE::
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
    UCk_UI_Layout_Subsystem_UE::
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
    auto SuspendHandle = UCk_Utils_UI_UE::SuspendInput(LocalPlayer, TEXT("AsyncWidgetLoad"));
    auto& StreamableManager = UAssetManager::GetStreamableManager();

    StreamableManager.RequestAsyncLoad(
        InWidgetClass.ToSoftObjectPath(),
        FStreamableDelegate::CreateWeakLambda(this,
            [this, InLayerTag, InWidgetClass, InOnWidgetReady, SuspendHandle]() mutable
        {
            SuspendHandle.Resume();

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
    UCk_UI_Layout_Subsystem_UE::
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
    UCk_UI_Layout_Subsystem_UE::
    PopWidgetFromLayer(
        FGameplayTag InLayerTag)
    -> UCommonActivatableWidget*
{
    if (ck::Is_NOT_Valid(_Layout))
    { return nullptr; }

    return _Layout->PopWidgetFromLayer(InLayerTag);
}

auto
    UCk_UI_Layout_Subsystem_UE::
    ClearLayer(
        FGameplayTag InLayerTag)
    -> void
{
    if (ck::Is_NOT_Valid(_Layout))
    { return; }

    _Layout->ClearLayer(InLayerTag);
}

auto
    UCk_UI_Layout_Subsystem_UE::
    RemoveWidget(
        UCommonActivatableWidget* InWidget)
    -> bool
{
    if (ck::Is_NOT_Valid(_Layout))
    { return false; }

    return _Layout->RemoveWidget(InWidget);
}

// --------------------------------------------------------------------------------------------------------------------
// Input Mode Query
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Layout_Subsystem_UE::
    Get_EffectiveInputMode() const
    -> ECk_UI_InputMode
{
    if (ck::Is_NOT_Valid(_Layout))
    { return ECk_UI_InputMode::GameOnly; }

    return _Layout->Get_EffectiveInputMode();
}

// --------------------------------------------------------------------------------------------------------------------
// Internal
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Layout_Subsystem_UE::
    DoCreateLayout(
        UCk_UI_LayoutConfigAsset_UE* InConfigAsset)
    -> void
{
    if (ck::IsValid(_Layout))
    { return; }

    const auto* LocalPlayer = GetLocalPlayer();

    CK_ENSURE_IF_NOT(ck::IsValid(LocalPlayer),
        TEXT("Layout Subsystem has no LocalPlayer during layout creation"))
    { return; }

    auto* PlayerController = LocalPlayer->GetPlayerController(GetWorld());

    CK_ENSURE_IF_NOT(ck::IsValid(PlayerController),
        TEXT("Layout Subsystem LocalPlayer has no PlayerController during layout creation"))
    { return; }

    _Layout = CreateWidget<UCk_UI_PrimaryGameLayout_UE>(PlayerController);

    CK_ENSURE_IF_NOT(ck::IsValid(_Layout),
        TEXT("Failed to create Primary Game Layout widget"))
    { return; }

    _Layout->InitializeFromConfig(InConfigAsset);
    _Layout->AddToViewport(InConfigAsset->Get_LayoutZOrder());
    DoBindLayoutEvents();
}

auto
    UCk_UI_Layout_Subsystem_UE::
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
    UCk_UI_Layout_Subsystem_UE::
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
    UCk_UI_Layout_Subsystem_UE::
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
    UCk_UI_Layout_Subsystem_UE::
    DoLoadAndPushStartingWidgets(
        UCk_UI_LayoutConfigAsset_UE* InConfigAsset)
    -> void
{
    if (ck::Is_NOT_Valid(InConfigAsset))
    {
        OnLayoutCreated.Broadcast();
        OnLayoutCreated_BP.Broadcast();
        return;
    }

    TArray<FSoftObjectPath> PathsToLoad;

    ck::algo::ForEach(InConfigAsset->Get_LayerConfigs(), [&PathsToLoad](const FCk_UI_LayerConfig& InConfig)
    {
        ck::algo::ForEachIsValid(InConfig.Get_StartingWidgetClasses(),
            [&PathsToLoad](const TSoftClassPtr<UCommonActivatableWidget>& InWidgetClass)
        {
            PathsToLoad.Add(InWidgetClass.ToSoftObjectPath());
        });
    });

    if (PathsToLoad.IsEmpty())
    {
        OnLayoutCreated.Broadcast();
        OnLayoutCreated_BP.Broadcast();
        return;
    }

    const auto* LocalPlayer = GetLocalPlayer();
    auto SuspendHandle = UCk_Utils_UI_UE::SuspendInput(LocalPlayer, TEXT("LoadStartingWidgets"));
    auto& StreamableManager = UAssetManager::GetStreamableManager();

    StreamableManager.RequestAsyncLoad(
        PathsToLoad,
        FStreamableDelegate::CreateWeakLambda(this,
            [this, InConfigAsset, SuspendHandle]() mutable
        {
            SuspendHandle.Resume();

            if (ck::Is_NOT_Valid(_Layout))
            { return; }

            ck::algo::ForEach(InConfigAsset->Get_LayerConfigs(), [this](const FCk_UI_LayerConfig& InConfig)
            {
                ck::algo::ForEachIsValid(InConfig.Get_StartingWidgetClasses(),
                    [this, &InConfig](const TSoftClassPtr<UCommonActivatableWidget>& InWidgetClass)
                {
                    _Layout->PushWidgetToLayer(InConfig.Get_LayerTag(), InWidgetClass.Get());
                });
            });

            OnLayoutCreated.Broadcast();
            OnLayoutCreated_BP.Broadcast();
        }));
}

auto
    UCk_UI_Layout_Subsystem_UE::
    DoLoadAndRegisterHUDElements(
        UCk_UI_LayoutConfigAsset_UE* InConfigAsset)
    -> void
{
    if (ck::Is_NOT_Valid(InConfigAsset))
    { return; }

    const auto& HUDElements = InConfigAsset->Get_HUDElements();

    if (HUDElements.IsEmpty())
    { return; }

    TArray<FSoftObjectPath> PathsToLoad;

    ck::algo::ForEach(HUDElements, [&PathsToLoad](const FCk_UI_HUDElementEntry& InEntry)
    {
        if (ck::IsValid(InEntry.Get_WidgetClass()))
        {
            PathsToLoad.Add(InEntry.Get_WidgetClass().ToSoftObjectPath());
        }
    });

    if (PathsToLoad.IsEmpty())
    { return; }

    const auto* LocalPlayer = GetLocalPlayer();
    auto SuspendHandle = UCk_Utils_UI_UE::SuspendInput(LocalPlayer, TEXT("LoadHUDElements"));
    auto& StreamableManager = UAssetManager::GetStreamableManager();

    StreamableManager.RequestAsyncLoad(
        PathsToLoad,
        FStreamableDelegate::CreateWeakLambda(this,
            [this, InConfigAsset, SuspendHandle]() mutable
        {
            SuspendHandle.Resume();

            auto* ExtensionSubsystem = GetLocalPlayer()->GetSubsystem<UCk_UI_Extension_Subsystem_UE>();

            if (ck::Is_NOT_Valid(ExtensionSubsystem))
            { return; }

            ck::algo::ForEach(InConfigAsset->Get_HUDElements(),
                [this, ExtensionSubsystem](const FCk_UI_HUDElementEntry& InEntry)
            {
                const auto LoadedClass = InEntry.Get_WidgetClass().Get();

                if (ck::Is_NOT_Valid(LoadedClass))
                { return; }

                const auto& Handle = ExtensionSubsystem->RegisterExtension(
                    InEntry.Get_SlotTag(),
                    LoadedClass,
                    InEntry.Get_Priority());

                if (ck::IsValid(Handle))
                {
                    _HUDElementHandles.Add(Handle);
                }
            });
        }));
}

auto
    UCk_UI_Layout_Subsystem_UE::
    DoUnregisterHUDElements()
    -> void
{
    for (auto& Handle : _HUDElementHandles)
    {
        Handle.Unregister();
    }

    _HUDElementHandles.Empty();
}

auto
    UCk_UI_Layout_Subsystem_UE::
    HandleLayoutWidgetPushed(
        FGameplayTag InLayerTag,
        UCommonActivatableWidget* InWidget) const
    -> void
{
    OnWidgetPushed.Broadcast(InWidget);
}

auto
    UCk_UI_Layout_Subsystem_UE::
    HandleLayoutWidgetPopped(
        FGameplayTag InLayerTag,
        UCommonActivatableWidget* InWidget) const
    -> void
{
    OnWidgetPopped.Broadcast(InWidget);
}

auto
    UCk_UI_Layout_Subsystem_UE::
    HandleLayoutLayerCleared(
        FGameplayTag InLayerTag) const
    -> void
{
    OnLayerCleared.Broadcast();
}

auto
    UCk_UI_Layout_Subsystem_UE::
    HandleLayoutInputModeChanged(
        ECk_UI_InputMode InNewMode) const
    -> void
{
    OnInputModeChanged.Broadcast(InNewMode);
}

// --------------------------------------------------------------------------------------------------------------------