// Copyright 2025 CkFoundation. All Rights Reserved.

#include "CkUI/Extension/CkUI_ExtensionPoint_Widget.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkUI/Extension/CkUI_Extension_Subsystem.h"

#include <Blueprint/UserWidget.h>
#include <Widgets/SOverlay.h>
#include <Widgets/Text/STextBlock.h>

#if WITH_EDITOR
#include <Misc/DataValidation.h>
#include <Misc/UObjectToken.h>
#include <Editor/WidgetCompilerLog.h>
#endif

#define LOCTEXT_NAMESPACE "CkUI_Extension"

// --------------------------------------------------------------------------------------------------------------------
// Constructor
// --------------------------------------------------------------------------------------------------------------------

UCk_UI_ExtensionPoint_Widget_UE::UCk_UI_ExtensionPoint_Widget_UE(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

// --------------------------------------------------------------------------------------------------------------------
// UWidget Overrides
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_ExtensionPoint_Widget_UE::
    ReleaseSlateResources(
        bool InReleaseChildren)
    -> void
{
    DoResetExtensionPoint();
    Super::ReleaseSlateResources(InReleaseChildren);
}

auto
    UCk_UI_ExtensionPoint_Widget_UE::
    RebuildWidget()
    -> TSharedRef<SWidget>
{
    if (IsDesignTime())
    {
        const auto GetExtensionPointText = [this]()
        {
            return FText::Format(
                LOCTEXT("DesignTime_Label", "Extension Point\n{0}"),
                FText::FromName(_ExtensionPointTag.GetTagName()));
        };

        auto MessageBox = SNew(SOverlay);

        MessageBox->AddSlot()
            .Padding(5.0f)
            .HAlign(HAlign_Center)
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Justification(ETextJustify::Center)
                .Text_Lambda(GetExtensionPointText)
            ];

        return MessageBox;
    }

    DoResetExtensionPoint();

    if (_ExtensionPointTag.IsValid())
    {
        DoRegisterExtensionPoint();
    }

    return Super::RebuildWidget();
}

#if WITH_EDITOR
auto
    UCk_UI_ExtensionPoint_Widget_UE::
    ValidateCompiledDefaults(
        IWidgetCompilerLog& InCompileLog) const
    -> void
{
    Super::ValidateCompiledDefaults(InCompileLog);

    if (HasAnyFlags(RF_ClassDefaultObject))
    { return; }

    if (ck::Is_NOT_Valid(_ExtensionPointTag))
    {
        const auto Message = InCompileLog.Error(FText::Format(
            LOCTEXT("NoTag_Error", "{0} has no ExtensionPointTag - all extension points must specify a tag"),
            FText::FromString(GetName())));
        Message->AddToken(FUObjectToken::Create(this));
    }
}
#endif

// --------------------------------------------------------------------------------------------------------------------
// Internal
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_ExtensionPoint_Widget_UE::
    DoResetExtensionPoint()
    -> void
{
    ResetInternal();
    _ExtensionMapping.Reset();
    _ExtensionPointHandle.Unregister();
}

auto
    UCk_UI_ExtensionPoint_Widget_UE::
    DoRegisterExtensionPoint()
    -> void
{
    const auto* LocalPlayer = GetOwningLocalPlayer();

    if (ck::Is_NOT_Valid(LocalPlayer))
    { return; }

    auto* ExtensionSubsystem = LocalPlayer->GetSubsystem<UCk_UI_Extension_Subsystem_UE>();

    if (ck::Is_NOT_Valid(ExtensionSubsystem))
    { return; }

    auto Callback = FCk_Delegate_ExtensionPoint_OnExtend::CreateUObject(
        this, &ThisClass::HandleExtensionAddedOrRemoved);

    _ExtensionPointHandle = ExtensionSubsystem->RegisterExtensionPoint(
        _ExtensionPointTag,
        _MatchType,
        Callback);
}

auto
    UCk_UI_ExtensionPoint_Widget_UE::
    DoGetExtensionSubsystem() const
    -> UCk_UI_Extension_Subsystem_UE*
{
    const auto* LocalPlayer = GetOwningLocalPlayer();

    if (ck::Is_NOT_Valid(LocalPlayer))
    { return nullptr; }

    return LocalPlayer->GetSubsystem<UCk_UI_Extension_Subsystem_UE>();
}

auto
    UCk_UI_ExtensionPoint_Widget_UE::
    HandleExtensionAddedOrRemoved(
        ECk_UI_ExtensionAction InAction,
        const FCk_UI_ExtensionRequest& InRequest)
    -> void
{
    if (InAction == ECk_UI_ExtensionAction::Added)
    {
        auto* InstanceWidget = InRequest.Get_WidgetInstance().Get();
        const auto HasInstance = ck::IsValid(InstanceWidget);

        if (NOT HasInstance && ck::Is_NOT_Valid(InRequest.Get_WidgetClass()))
        { return; }

        if (HasInstance)
        {
            AddEntryChild(*InstanceWidget);
        }

        auto* Widget = HasInstance
            ? InstanceWidget
            : CreateEntryInternal(InRequest.Get_WidgetClass());

        _ExtensionMapping.Add(InRequest.Get_ExtensionHandle(), Widget);

        if (auto* Subsystem = DoGetExtensionSubsystem();
            ck::IsValid(Subsystem))
        {
            Subsystem->NotifyWidgetCreatedForExtension(Widget, InRequest.Get_ExtensionHandle());
        }

        return;
    }

    // ---- Removed

    auto Widget = _ExtensionMapping.FindRef(InRequest.Get_ExtensionHandle());

    if (ck::Is_NOT_Valid(Widget))
    { return; }

    if (auto* Subsystem = DoGetExtensionSubsystem();
        ck::IsValid(Subsystem))
    {
        Subsystem->NotifyWidgetRemovedForExtension(Widget);
    }

    RemoveEntryInternal(Widget);
    _ExtensionMapping.Remove(InRequest.Get_ExtensionHandle());
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE