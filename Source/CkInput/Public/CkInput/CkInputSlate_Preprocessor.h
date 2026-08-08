#pragma once

#include "CkInput/CkInputSource_Fragment_Data.h"

#include <Framework/Application/IInputProcessor.h>
#include <UObject/WeakObjectPtrTemplates.h>

// --------------------------------------------------------------------------------------------------------------------

class UGameInstance;

// --------------------------------------------------------------------------------------------------------------------

/**
 * Records the raw device events Slate delivers into the owning local player's input-source inbox, and does
 * nothing else with them.
 *
 * Observe-only: every handler returns false. Consumption belongs to ECS routing, which is what lets this share
 * Slate index 0 with the loading screen's and the debugger's pre-processors without any of them arbitrating
 * against another.
 *
 * Recording is gated on this game instance's viewport holding user focus. That is what keeps the editor's own
 * keystrokes out of a source during PIE, and what stops a background PIE window from recording the foreground
 * window's input — Slate pre-processors are application-global, while an input source is one local player's.
 */
class FCk_InputSlate_Preprocessor : public IInputProcessor
{
public:
    explicit FCk_InputSlate_Preprocessor(
        UGameInstance* InGameInstance);

public:
    virtual void Tick(
        const float InDeltaTime,
        FSlateApplication& InSlateApp,
        TSharedRef<ICursor> InCursor) override;

    virtual bool HandleKeyDownEvent(
        FSlateApplication& InSlateApp,
        const FKeyEvent& InKeyEvent) override;

    virtual bool HandleKeyUpEvent(
        FSlateApplication& InSlateApp,
        const FKeyEvent& InKeyEvent) override;

    virtual bool HandleAnalogInputEvent(
        FSlateApplication& InSlateApp,
        const FAnalogInputEvent& InAnalogInputEvent) override;

    virtual bool HandleMouseMoveEvent(
        FSlateApplication& InSlateApp,
        const FPointerEvent& InMouseEvent) override;

    virtual bool HandleMouseButtonDownEvent(
        FSlateApplication& InSlateApp,
        const FPointerEvent& InMouseEvent) override;

    virtual bool HandleMouseButtonUpEvent(
        FSlateApplication& InSlateApp,
        const FPointerEvent& InMouseEvent) override;

    virtual const TCHAR* GetDebugName() const override;

private:
    auto
    DoRecordEvent(
        const FKey& InKey,
        ECk_InputSource_EventType InEventType,
        float InAnalogValue,
        int32 InRawDeviceUserIndex) -> void;

    auto
    DoGet_HasViewportFocus() const -> bool;

    auto
    DoTryGet_RoutedSource(
        const FCk_InputSource_DeviceId& InDeviceId) const -> FCk_Handle_InputSource;

    auto
    DoTryGet_SourceForLocalPlayer(
        int32 InLocalPlayerIndex) const -> FCk_Handle_InputSource;

private:
    TWeakObjectPtr<UGameInstance> _GameInstance;
};

// --------------------------------------------------------------------------------------------------------------------
