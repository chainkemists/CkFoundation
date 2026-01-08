// Copyright 2025 CkFoundation. All Rights Reserved.

#include "CkUI/Layout/CkUI_Layout.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"
#include "CkCore/Time/CkTime.h"
#include "CkUI/CkUI_Utils.h"
#include "CkUI/Layer/CkUI_LayerWidget.h"
#include "CkUI/Layer/CkUI_LayerStack.h"
#include "CkUI/Layer/CkUI_LayerConfigAsset.h"

#include <Blueprint/WidgetTree.h>
#include <Components/Overlay.h>
#include <Components/OverlaySlot.h>

// --------------------------------------------------------------------------------------------------------------------
// Initialization
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Layout_UE::
    InitializeFromConfig(
        UCk_UI_LayerConfigAsset_UE* InConfigAsset)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InConfigAsset),
        TEXT("Cannot initialize layout [{}] with invalid config asset"), this)
    { return; }

    DoCreateRootOverlay();
    DoCreateLayers(InConfigAsset);
}

// --------------------------------------------------------------------------------------------------------------------
// Layer Access
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Layout_UE::
    Get_Layer(
        FGameplayTag InLayerTag) const
    -> UCk_UI_LayerStack_UE*
{
    const auto* FoundLayer = _Layers.Find(InLayerTag);

    if (FoundLayer == nullptr)
    { return nullptr; }

    return *FoundLayer;
}

auto
    UCk_UI_Layout_UE::
    Get_AllLayers() const
    -> TArray<UCk_UI_LayerStack_UE*>
{
    TArray<UCk_UI_LayerStack_UE*> Result;
    Result.Reserve(_Layers.Num());

    for (const auto& [Tag, Layer] : _Layers)
    {
        Result.Add(Layer);
    }

    return Result;
}

auto
    UCk_UI_Layout_UE::
    HasLayer(
        FGameplayTag InLayerTag) const
    -> bool
{
    return _Layers.Contains(InLayerTag);
}

// --------------------------------------------------------------------------------------------------------------------
// Widget Operations
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Layout_UE::
    PushWidgetToLayer(
        FGameplayTag InLayerTag,
        TSubclassOf<UCommonActivatableWidget> InWidgetClass)
    -> UCommonActivatableWidget*
{
    auto* Layer = Get_Layer(InLayerTag);

    CK_ENSURE_IF_NOT(ck::IsValid(Layer),
        TEXT("Cannot push widget to unknown layer [{}]"), InLayerTag.ToString())
    { return nullptr; }

    return Layer->PushWidgetClass(InWidgetClass);
}

auto
    UCk_UI_Layout_UE::
    PushWidgetInstanceToLayer(
        FGameplayTag InLayerTag,
        UCommonActivatableWidget* InWidget)
    -> UCommonActivatableWidget*
{
    auto* Layer = Get_Layer(InLayerTag);

    CK_ENSURE_IF_NOT(ck::IsValid(Layer),
        TEXT("Cannot push widget instance to unknown layer [{}]"), InLayerTag.ToString())
    { return nullptr; }

    return Layer->PushWidgetInstance(InWidget);
}

auto
    UCk_UI_Layout_UE::
    PopWidgetFromLayer(
        FGameplayTag InLayerTag)
    -> UCommonActivatableWidget*
{
    auto* Layer = Get_Layer(InLayerTag);

    if (ck::Is_NOT_Valid(Layer))
    { return nullptr; }

    return Layer->PopWidget();
}

auto
    UCk_UI_Layout_UE::
    ClearLayer(
        FGameplayTag InLayerTag)
    -> void
{
    auto* Layer = Get_Layer(InLayerTag);

    if (ck::Is_NOT_Valid(Layer))
    { return; }

    Layer->ClearWidgets();
    OnLayerCleared.Broadcast(InLayerTag);
}

auto
    UCk_UI_Layout_UE::
    RemoveWidget(
        UCommonActivatableWidget* InWidget)
    -> bool
{
    if (ck::Is_NOT_Valid(InWidget))
    { return false; }

    for (const auto& [Tag, Layer] : _Layers)
    {
        if (Layer->PopSpecificWidget(InWidget))
        { return true; }
    }

    return false;
}

// --------------------------------------------------------------------------------------------------------------------
// Input Mode
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Layout_UE::
    Get_EffectiveInputMode() const
    -> ECk_UI_InputMode
{
    return _CachedInputMode.Get(ECk_UI_InputMode::GameOnly);
}

// --------------------------------------------------------------------------------------------------------------------
// Transition State
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Layout_UE::
    IsAnyLayerTransitioning() const
    -> bool
{
    return _TransitionSuspendTokens.Num() > 0;
}

// --------------------------------------------------------------------------------------------------------------------
// UUserWidget Overrides
// --------------------------------------------------------------------------------------------------------------------

void
    UCk_UI_Layout_UE::
    NativeConstruct()
{
    Super::NativeConstruct();
    ActivateWidget();
}

void
    UCk_UI_Layout_UE::
    NativeDestruct()
{
    // Resume any outstanding transition suspensions before destruction
    const auto* LocalPlayer = GetOwningLocalPlayer();

    for (const auto& Token : _TransitionSuspendTokens)
    {
        UCk_Utils_UI_UE::ResumeInput(LocalPlayer, Token);
    }

    _TransitionSuspendTokens.Empty();

    DoDestroyLayers();
    Super::NativeDestruct();
}

auto
    UCk_UI_Layout_UE::
    RebuildWidget()
    -> TSharedRef<SWidget>
{
    if (ck::Is_NOT_Valid(WidgetTree))
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    }

    if (ck::Is_NOT_Valid(_RootOverlay) && ck::IsValid(WidgetTree))
    {
        _RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("RootOverlay"));
        WidgetTree->RootWidget = _RootOverlay;
    }

    return Super::RebuildWidget();
}

// --------------------------------------------------------------------------------------------------------------------
// Internal - Layer Management
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Layout_UE::
    DoCreateRootOverlay()
    -> void
{
    if (ck::IsValid(_RootOverlay))
    { return; }

    if (ck::Is_NOT_Valid(WidgetTree))
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    }

    _RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("RootOverlay"));

    CK_ENSURE_IF_NOT(ck::IsValid(_RootOverlay),
        TEXT("Failed to create root overlay for layout [{}]"), this)
    { return; }

    WidgetTree->RootWidget = _RootOverlay;
}

auto
    UCk_UI_Layout_UE::
    DoCreateLayers(
        const UCk_UI_LayerConfigAsset_UE* InConfigAsset)
    -> void
{
    const auto& LayerConfigs = InConfigAsset->Get_LayerConfigs();

    struct FLayerCreationEntry
    {
        FCk_UI_LayerConfig Config;
        TObjectPtr<UCk_UI_LayerWidget_UE> Wrapper;
    };

    const auto& ValidConfigs = ck::algo::Filter(LayerConfigs, [](const FCk_UI_LayerConfig& InConfig)
    {
        return ck::IsValid(InConfig.Get_LayerTag());
    });

    const auto& CreationEntries = ck::algo::Transform<TArray<FLayerCreationEntry>>(ValidConfigs,
        [this](const FCk_UI_LayerConfig& InConfig) -> FLayerCreationEntry
        {
            auto* Wrapper = WidgetTree->ConstructWidget<UCk_UI_LayerWidget_UE>(
                UCk_UI_LayerWidget_UE::StaticClass(),
                *ck::Format_UE(TEXT("LayerWrapper_{}"), InConfig.Get_LayerTag().ToString()));

            if (ck::Is_NOT_Valid(Wrapper))
            { return { InConfig, nullptr }; }

            auto StackClass = InConfig.Get_LayerWidgetClass();

            if (ck::Is_NOT_Valid(StackClass))
            {
                StackClass = UCk_UI_LayerStack_UE::StaticClass();
            }

            Wrapper->Configure(
                StackClass,
                InConfig.Get_LayerTag(),
                InConfig.Get_Priority(),
                InConfig.Get_InputMode());

            if (InConfig.Get_TransitionDuration() > 0.0f)
            {
                Wrapper->SetTransitionSettings(
                    InConfig.Get_TransitionType(),
                    InConfig.Get_TransitionCurve(),
                    FCk_Time(InConfig.Get_TransitionDuration()));
            }

            return { InConfig, Wrapper };
        });

    const auto& ValidEntries = ck::algo::Filter(CreationEntries, [](const FLayerCreationEntry& InEntry)
    {
        return ck::IsValid(InEntry.Wrapper);
    });

    auto SortedEntries = ck::algo::Sort(ValidEntries, [](const FLayerCreationEntry& A, const FLayerCreationEntry& B)
    {
        return A.Config.Get_Priority() < B.Config.Get_Priority();
    });

    ck::algo::ForEach(SortedEntries, [this](const FLayerCreationEntry& InEntry)
    {
        _LayerWrappers.Add(InEntry.Wrapper);
        DoAddLayerToOverlay(InEntry.Wrapper);
        DoBindLayerEvents(InEntry.Wrapper);
    });

    // Force widget construction and register layers
    // This must happen after all wrappers are added to the overlay
    for (const auto& Wrapper : _LayerWrappers)
    {
        if (ck::Is_NOT_Valid(Wrapper))
        { continue; }

        Wrapper->TakeWidget();
        DoRegisterLayer(Wrapper);
    }
}

auto
    UCk_UI_Layout_UE::
    DoRegisterLayer(
        UCk_UI_LayerWidget_UE* InWrapper)
    -> void
{
    if (ck::Is_NOT_Valid(InWrapper))
    { return; }

    auto* Stack = InWrapper->Get_Stack();

    if (ck::Is_NOT_Valid(Stack))
    { return; }

    _Layers.Add(InWrapper->Get_LayerTag(), Stack);
}

auto
    UCk_UI_Layout_UE::
    DoDestroyLayers()
    -> void
{
    for (const auto& Wrapper : _LayerWrappers)
    {
        DoUnbindLayerEvents(Wrapper);

        if (auto* Stack = Wrapper->Get_Stack(); ck::IsValid(Stack))
        {
            Stack->ClearWidgets();
        }
    }

    _Layers.Empty();
    _LayerWrappers.Empty();
}

auto
    UCk_UI_Layout_UE::
    DoAddLayerToOverlay(
        UCk_UI_LayerWidget_UE* InWrapper)
    -> void
{
    if (ck::Is_NOT_Valid(_RootOverlay))
    { return; }

    if (ck::Is_NOT_Valid(InWrapper))
    { return; }

    auto* OverlaySlot = Cast<UOverlaySlot>(_RootOverlay->AddChild(InWrapper));

    if (ck::Is_NOT_Valid(OverlaySlot))
    { return; }

    OverlaySlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Fill);
    OverlaySlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Fill);
}

auto
    UCk_UI_Layout_UE::
    DoBindLayerEvents(
        UCk_UI_LayerWidget_UE* InWrapper)
    -> void
{
    if (ck::Is_NOT_Valid(InWrapper))
    { return; }

    InWrapper->OnWidgetPushed.AddUObject(this, &ThisClass::HandleLayerWidgetPushed, InWrapper);
    InWrapper->OnWidgetPopped.AddUObject(this, &ThisClass::HandleLayerWidgetPopped, InWrapper);
    InWrapper->OnTransitionStateChanged.AddUObject(this, &ThisClass::HandleLayerTransitionStateChanged, InWrapper);
}

auto
    UCk_UI_Layout_UE::
    DoUnbindLayerEvents(
        UCk_UI_LayerWidget_UE* InWrapper)
    -> void
{
    if (ck::Is_NOT_Valid(InWrapper))
    { return; }

    InWrapper->OnWidgetPushed.RemoveAll(this);
    InWrapper->OnWidgetPopped.RemoveAll(this);
    InWrapper->OnTransitionStateChanged.RemoveAll(this);
}

auto
    UCk_UI_Layout_UE::
    DoUpdateInputMode()
    -> void
{
    auto NewMode = ECk_UI_InputMode::GameOnly;

    for (const auto& Wrapper : _LayerWrappers)
    {
        if (ck::Is_NOT_Valid(Wrapper))
        { continue; }

        const auto* Stack = Wrapper->Get_Stack();

        if (ck::Is_NOT_Valid(Stack))
        { continue; }

        if (Stack->HasWidgets())
        {
            NewMode = Wrapper->Get_InputMode();
        }
    }

    if (_CachedInputMode.IsSet() && NewMode == _CachedInputMode.GetValue())
    { return; }

    _CachedInputMode = NewMode;
    OnInputModeChanged.Broadcast(NewMode);
}

// --------------------------------------------------------------------------------------------------------------------
// Internal - Transition Handling
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Layout_UE::
    DoHandleTransitionStateChanged(
        UCk_UI_LayerWidget_UE* InWrapper,
        bool InIsTransitioning)
    -> void
{
    const auto* LocalPlayer = GetOwningLocalPlayer();

    if (InIsTransitioning)
    {
        const auto SuspendToken = UCk_Utils_UI_UE::SuspendInput(LocalPlayer, TEXT("LayerTransition"));
        _TransitionSuspendTokens.Add(SuspendToken);
    }
    else
    {
        CK_ENSURE_IF_NOT(_TransitionSuspendTokens.Num() > 0,
            TEXT("Transition ended but no suspend tokens exist - mismatched start/end calls"))
        { return; }

        const auto SuspendToken = _TransitionSuspendTokens.Pop();
        UCk_Utils_UI_UE::ResumeInput(LocalPlayer, SuspendToken);
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Internal - Event Handlers
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Layout_UE::
    HandleLayerWidgetPushed(
        UCommonActivatableWidget* InWidget,
        UCk_UI_LayerWidget_UE* InWrapper)
    -> void
{
    OnWidgetPushed.Broadcast(InWrapper->Get_LayerTag(), InWidget);
    DoUpdateInputMode();
}

auto
    UCk_UI_Layout_UE::
    HandleLayerWidgetPopped(
        UCommonActivatableWidget* InWidget,
        UCk_UI_LayerWidget_UE* InWrapper)
    -> void
{
    OnWidgetPopped.Broadcast(InWrapper->Get_LayerTag(), InWidget);
    DoUpdateInputMode();
}

auto
    UCk_UI_Layout_UE::
    HandleLayerTransitionStateChanged(
        bool InIsTransitioning,
        UCk_UI_LayerWidget_UE* InWrapper)
    -> void
{
    DoHandleTransitionStateChanged(InWrapper, InIsTransitioning);
}

// --------------------------------------------------------------------------------------------------------------------