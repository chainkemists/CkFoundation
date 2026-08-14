#include "CkInputSlate_Preprocessor.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkInput/CkInput_Log.h"
#include "CkInput/CkInputSource_Utils.h"
#include "CkInput/Subsystem/CkInputSource_Subsystem.h"

#include <CoreGlobals.h>
#include <Engine/GameInstance.h>
#include <Engine/GameViewportClient.h>
#include <Engine/LocalPlayer.h>
#include <HAL/IConsoleManager.h>
#include <Input/Events.h>
#include <Widgets/SViewport.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_input_slate_preprocessor
{
    static bool DumpRawEvents = false;
    static FAutoConsoleVariableRef CVarDumpRawEvents(
        TEXT("ck.Input.DumpRawEvents"),
        DumpRawEvents,
        TEXT("When true, every raw device event the Slate writer records is logged with its frame, device "
             "class, key, event type, analog value, raw device user index, ordering fidelity and destination "
             "input source. Off by default."),
        ECVF_Default);

    constexpr auto KeyboardAndMouseLocalPlayerIndex = 0;

    auto Get_DeviceClassForKey(
        const FKey& InKey)
        -> ECk_InputSource_DeviceClass
    {
        if (InKey.IsGamepadKey())
        { return ECk_InputSource_DeviceClass::Gamepad; }

        if (InKey.IsMouseButton())
        { return ECk_InputSource_DeviceClass::Mouse; }

        return ECk_InputSource_DeviceClass::Keyboard;
    }

    auto Get_OrderingFidelityForDeviceClass(
        ECk_InputSource_DeviceClass InDeviceClass)
        -> ECk_InputSource_OrderingFidelity
    {
        // Keyboard and mouse events arrive off the OS message queue, which preserves their order within a
        // frame; a gamepad is read as a polled bitmask, so its events only share a frame and cannot be ranked
        // inside it.
        return InDeviceClass == ECk_InputSource_DeviceClass::Gamepad
            ? ECk_InputSource_OrderingFidelity::Simultaneous
            : ECk_InputSource_OrderingFidelity::SubFrameOrdered;
    }
}

// --------------------------------------------------------------------------------------------------------------------

FCk_InputSlate_Preprocessor::
    FCk_InputSlate_Preprocessor(
        UGameInstance* InGameInstance)
    : _GameInstance(InGameInstance)
{
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_InputSlate_Preprocessor::
    Tick(
        const float,
        FSlateApplication&,
        TSharedRef<ICursor>)
    -> void
{
    // The probe only runs while something is recorded down — the common every-frame case exits on the
    // empty set.
    if (_RecordedDownKeys.IsEmpty())
    { return; }

    if (DoGet_HasViewportFocus())
    { return; }

    DoFlushRecordedDownKeys();
}

auto
    FCk_InputSlate_Preprocessor::
    HandleKeyDownEvent(
        FSlateApplication&,
        const FKeyEvent& InKeyEvent)
    -> bool
{
    // A held key is one Pressed until its Released — the OS auto-repeat stream carries no new physical edge.
    if (InKeyEvent.IsRepeat())
    { return false; }

    DoRecordEvent(InKeyEvent.GetKey(), ECk_InputSource_EventType::Pressed, 0.0f,
        static_cast<int32>(InKeyEvent.GetUserIndex()));

    return false;
}

auto
    FCk_InputSlate_Preprocessor::
    HandleKeyUpEvent(
        FSlateApplication&,
        const FKeyEvent& InKeyEvent)
    -> bool
{
    DoRecordEvent(InKeyEvent.GetKey(), ECk_InputSource_EventType::Released, 0.0f,
        static_cast<int32>(InKeyEvent.GetUserIndex()));

    return false;
}

auto
    FCk_InputSlate_Preprocessor::
    HandleAnalogInputEvent(
        FSlateApplication&,
        const FAnalogInputEvent& InAnalogInputEvent)
    -> bool
{
    DoRecordEvent(InAnalogInputEvent.GetKey(), ECk_InputSource_EventType::AnalogAxis,
        InAnalogInputEvent.GetAnalogValue(), static_cast<int32>(InAnalogInputEvent.GetUserIndex()));

    return false;
}

auto
    FCk_InputSlate_Preprocessor::
    HandleMouseMoveEvent(
        FSlateApplication&,
        const FPointerEvent& InMouseEvent)
    -> bool
{
    // Look is two independent 1D axes downstream, so the cursor delta is split into the same MouseX / MouseY
    // rows the rest of the engine names it by. A component that did not move is not a sample, so it writes no
    // row at all rather than a zero one.
    const auto CursorDelta = InMouseEvent.GetCursorDelta();
    const auto RawDeviceUserIndex = static_cast<int32>(InMouseEvent.GetUserIndex());

    if (CursorDelta.X != 0.0f)
    {
        DoRecordEvent(EKeys::MouseX, ECk_InputSource_EventType::AnalogAxis,
            static_cast<float>(CursorDelta.X), RawDeviceUserIndex);
    }

    if (CursorDelta.Y != 0.0f)
    {
        DoRecordEvent(EKeys::MouseY, ECk_InputSource_EventType::AnalogAxis,
            static_cast<float>(CursorDelta.Y), RawDeviceUserIndex);
    }

    return false;
}

auto
    FCk_InputSlate_Preprocessor::
    HandleMouseButtonDownEvent(
        FSlateApplication&,
        const FPointerEvent& InMouseEvent)
    -> bool
{
    DoRecordEvent(InMouseEvent.GetEffectingButton(), ECk_InputSource_EventType::Pressed, 0.0f,
        static_cast<int32>(InMouseEvent.GetUserIndex()));

    return false;
}

auto
    FCk_InputSlate_Preprocessor::
    HandleMouseButtonDoubleClickEvent(
        FSlateApplication&,
        const FPointerEvent& InMouseEvent)
    -> bool
{
    DoRecordEvent(InMouseEvent.GetEffectingButton(), ECk_InputSource_EventType::Pressed, 0.0f,
        static_cast<int32>(InMouseEvent.GetUserIndex()));

    return false;
}

auto
    FCk_InputSlate_Preprocessor::
    HandleMouseButtonUpEvent(
        FSlateApplication&,
        const FPointerEvent& InMouseEvent)
    -> bool
{
    DoRecordEvent(InMouseEvent.GetEffectingButton(), ECk_InputSource_EventType::Released, 0.0f,
        static_cast<int32>(InMouseEvent.GetUserIndex()));

    return false;
}

auto
    FCk_InputSlate_Preprocessor::
    HandleMouseWheelOrGestureEvent(
        FSlateApplication&,
        const FPointerEvent& InWheelEvent,
        const FPointerEvent* InGestureEvent)
    -> bool
{
    // A gesture-sourced scroll is surfaced by its gesture TYPE and carries no wheel key, so there is no identity
    // to record it under, and it is deliberately not recorded — this return precedes BOTH record calls below and
    // Slate makes no second dispatch of the same scroll without the gesture. The consequence is worth knowing
    // before anyone relies on the wheel: on a platform that routes all scrolling through gestures (a macOS
    // trackpad) the record gets neither a notch nor an axis row. Whether that should synthesize a wheel identity
    // is a design call, and it is the maintainer's.
    if (InGestureEvent != nullptr)
    { return false; }

    const auto WheelDelta = InWheelEvent.GetWheelDelta();
    if (WheelDelta == 0.0f)
    { return false; }

    const auto RawDeviceUserIndex = static_cast<int32>(InWheelEvent.GetUserIndex());

    DoRecordEvent(EKeys::MouseWheelAxis, ECk_InputSource_EventType::AnalogAxis, WheelDelta,
        RawDeviceUserIndex);

    // A notch has no duration — the engine surfaces MouseScrollUp/Down as a press and a release inside
    // one frame, and the record has to say the same thing. A press with no release would leave the
    // button held for the rest of the session in every consumer that tracks the held set.
    const auto NotchKey = WheelDelta > 0.0f ? EKeys::MouseScrollUp : EKeys::MouseScrollDown;
    DoRecordEvent(NotchKey, ECk_InputSource_EventType::Pressed, 0.0f, RawDeviceUserIndex);
    DoRecordEvent(NotchKey, ECk_InputSource_EventType::Released, 0.0f, RawDeviceUserIndex);

    return false;
}

auto
    FCk_InputSlate_Preprocessor::
    GetDebugName() const
    -> const TCHAR*
{
    return TEXT("CkInputSlate");
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_InputSlate_Preprocessor::
    DoRecordEvent(
        const FKey& InKey,
        ECk_InputSource_EventType InEventType,
        float InAnalogValue,
        int32 InRawDeviceUserIndex)
    -> void
{
    if (NOT InKey.IsValid())
    { return; }

    if (NOT DoGet_HasViewportFocus())
    { return; }

    DoWriteEvent(InKey, InEventType, InAnalogValue, InRawDeviceUserIndex);
}

// The focus-gate-free half of recording: the focus-loss flush has to write Released rows at exactly the
// moment the gate would drop them, so the gate lives in DoRecordEvent and everything below it lives here.
auto
    FCk_InputSlate_Preprocessor::
    DoWriteEvent(
        const FKey& InKey,
        ECk_InputSource_EventType InEventType,
        float InAnalogValue,
        int32 InRawDeviceUserIndex)
    -> void
{
    const auto DeviceClass = ck_input_slate_preprocessor::Get_DeviceClassForKey(InKey);
    const auto OrderingFidelity = ck_input_slate_preprocessor::Get_OrderingFidelityForDeviceClass(DeviceClass);

    auto RoutedSource = DoTryGet_RoutedSource(FCk_InputSource_DeviceId{DeviceClass, InRawDeviceUserIndex});
    if (ck::Is_NOT_Valid(RoutedSource))
    { return; }

    auto Event = FCk_InputSource_RawEvent{DeviceClass, InKey, InEventType};
    Event.Set_OrderingFidelity(OrderingFidelity)
         .Set_AnalogValue(InAnalogValue)
         .Set_RawDeviceUserIndex(InRawDeviceUserIndex);

    UCk_Utils_InputSource_UE::Request_InjectRawEvent(RoutedSource,
        FCk_Request_InputSource_InjectRawEvent{Event}, {});

    // Only a press that actually reached an inbox can leave a phantom behind, so tracking sits below the
    // routing early-outs.
    if (InEventType == ECk_InputSource_EventType::Pressed)
    { _RecordedDownKeys.Add({InKey, InRawDeviceUserIndex}); }
    else if (InEventType == ECk_InputSource_EventType::Released)
    { _RecordedDownKeys.Remove({InKey, InRawDeviceUserIndex}); }

    if (NOT ck_input_slate_preprocessor::DumpRawEvents)
    { return; }

    ck::input::Display(
        TEXT("[CkInputDump] Frame=[{}] Device=[{}] Key=[{}] Type=[{}] Analog=[{}] UserIndex=[{}] "
             "Ordering=[{}] Source=[{}]"),
        GFrameCounter, DeviceClass, InKey, InEventType, InAnalogValue, InRawDeviceUserIndex,
        OrderingFidelity, RoutedSource);
}

auto
    FCk_InputSlate_Preprocessor::
    DoFlushRecordedDownKeys()
    -> void
{
    // DoWriteEvent removes each Released from the set, so the walk runs over a moved-out copy.
    const auto DownKeys = MoveTemp(_RecordedDownKeys);

    for (const auto& [Key, RawDeviceUserIndex] : DownKeys)
    {
        DoWriteEvent(Key, ECk_InputSource_EventType::Released, 0.0f, RawDeviceUserIndex);
    }
}

auto
    FCk_InputSlate_Preprocessor::
    DoGet_HasViewportFocus() const
    -> bool
{
    const auto* GameInstance = _GameInstance.Get();
    if (ck::Is_NOT_Valid(GameInstance))
    { return false; }

    const auto* ViewportClient = GameInstance->GetGameViewportClient();
    if (ck::Is_NOT_Valid(ViewportClient))
    { return false; }

    const auto ViewportWidget = ViewportClient->GetGameViewportWidget();
    if (NOT ViewportWidget.IsValid())
    { return false; }

    // DIRECT focus only. The console (and any chat/text field hosted in the viewport's overlay) steals focus
    // to a DESCENDANT, and those keystrokes belong to the text field, not the game — descendant-focus recorded
    // "slomo 0.1" into the input record as gameplay presses. The focus-loss flush releases anything held when
    // a text field takes over, so the strictness cannot strand a phantom press.
    return ViewportWidget->HasAnyUserFocus().IsSet();
}

auto
    FCk_InputSlate_Preprocessor::
    DoTryGet_RoutedSource(
        const FCk_InputSource_DeviceId& InDeviceId) const
    -> FCk_Handle_InputSource
{
    auto DefaultSource = DoTryGet_SourceForLocalPlayer(
        ck_input_slate_preprocessor::KeyboardAndMouseLocalPlayerIndex);

    if (ck::Is_NOT_Valid(DefaultSource))
    { return {}; }

    if (auto AssignedSource = UCk_Utils_InputSource_UE::TryGet_SourceOwningDevice(
            DefaultSource.ConvertToHandle(), InDeviceId);
        ck::IsValid(AssignedSource))
    { return AssignedSource; }

    if (InDeviceId.Get_DeviceClass() != ECk_InputSource_DeviceClass::Gamepad)
    { return DefaultSource; }

    return DoTryGet_SourceForLocalPlayer(InDeviceId.Get_RawDeviceUserIndex());
}

auto
    FCk_InputSlate_Preprocessor::
    DoTryGet_SourceForLocalPlayer(
        int32 InLocalPlayerIndex) const
    -> FCk_Handle_InputSource
{
    const auto* GameInstance = _GameInstance.Get();
    if (ck::Is_NOT_Valid(GameInstance))
    { return {}; }

    const auto* LocalPlayer = GameInstance->GetLocalPlayerByIndex(InLocalPlayerIndex);
    if (ck::Is_NOT_Valid(LocalPlayer))
    { return {}; }

    auto* SourceSubsystem = LocalPlayer->GetSubsystem<UCk_InputSource_Subsystem>();
    if (ck::Is_NOT_Valid(SourceSubsystem))
    { return {}; }

    return SourceSubsystem->Get_InputSource();
}

// --------------------------------------------------------------------------------------------------------------------
