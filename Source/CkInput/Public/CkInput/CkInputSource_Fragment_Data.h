#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include <InputCoreTypes.h>

#include "CkInputSource_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_InputSource_DeviceClass : uint8
{
    Keyboard,
    Mouse,
    Gamepad
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_InputSource_DeviceClass);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_InputSource_EventType : uint8
{
    Pressed,
    Released,
    AnalogAxis
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_InputSource_EventType);

// --------------------------------------------------------------------------------------------------------------------

// Whether the producer of an event could tell where it sat in the arrival order relative to the other events
// of the same frame. This is carried PER EVENT rather than per source because one local player routinely owns
// several device classes at once (a keyboard arriving off the message queue is ordered; a gamepad read as a
// polled bitmask is not), so a single source-level flag would necessarily lie about one of them. Consumers
// that need ordering consult the events they are actually looking at and degrade to Simultaneous otherwise.
UENUM(BlueprintType)
enum class ECk_InputSource_OrderingFidelity : uint8
{
    Simultaneous,
    SubFrameOrdered
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_InputSource_OrderingFidelity);

// --------------------------------------------------------------------------------------------------------------------

// A physical device, identified by its class AND its raw platform user index together. Neither half identifies a
// device on its own: every keyboard reports index 0 regardless of which split-screen player is typing on it.
USTRUCT(BlueprintType)
struct CKINPUT_API FCk_InputSource_DeviceId
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_InputSource_DeviceId);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_InputSource_DeviceClass _DeviceClass = ECk_InputSource_DeviceClass::Keyboard;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _RawDeviceUserIndex = 0;

public:
    CK_PROPERTY_GET(_DeviceClass);
    CK_PROPERTY_GET(_RawDeviceUserIndex);

public:
    auto operator==(
        const FCk_InputSource_DeviceId& InOther) const -> bool
    {
        return _DeviceClass == InOther._DeviceClass && _RawDeviceUserIndex == InOther._RawDeviceUserIndex;
    }

    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(FCk_InputSource_DeviceId);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_InputSource_DeviceId, _DeviceClass, _RawDeviceUserIndex);
};

// --------------------------------------------------------------------------------------------------------------------

// One raw device event exactly as its producer saw it. Analog axes are recorded verbatim — no deadzone, no
// curve, no filtering — because conditioning is a later, per-game stage and a row that was already conditioned
// cannot be un-conditioned by anything downstream.
USTRUCT(BlueprintType)
struct CKINPUT_API FCk_InputSource_RawEvent
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_InputSource_RawEvent);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_InputSource_DeviceClass _DeviceClass = ECk_InputSource_DeviceClass::Keyboard;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FKey _Key;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_InputSource_EventType _EventType = ECk_InputSource_EventType::Pressed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_InputSource_OrderingFidelity _OrderingFidelity = ECk_InputSource_OrderingFidelity::Simultaneous;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _AnalogValue = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _RawDeviceUserIndex = 0;

public:
    CK_PROPERTY_GET(_DeviceClass);
    CK_PROPERTY_GET(_Key);
    CK_PROPERTY_GET(_EventType);
    CK_PROPERTY(_OrderingFidelity);
    CK_PROPERTY(_AnalogValue);
    CK_PROPERTY(_RawDeviceUserIndex);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_InputSource_RawEvent, _DeviceClass, _Key, _EventType);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta = (HasNativeMake, HasNativeBreak))
struct CKINPUT_API FCk_Handle_InputSource : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_InputSource); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_InputSource);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINPUT_API FCk_Fragment_InputSource_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_InputSource_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _LocalPlayerIndex = 0;

public:
    CK_PROPERTY_GET(_LocalPlayerIndex);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_InputSource_ParamsData, _LocalPlayerIndex);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINPUT_API FCk_Request_InputSource_InjectRawEvent : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_InputSource_InjectRawEvent);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_InputSource_InjectRawEvent);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_InputSource_RawEvent _Event;

public:
    CK_PROPERTY_GET(_Event);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_InputSource_InjectRawEvent, _Event);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINPUT_API FCk_Request_InputSource_AssignDevice : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_InputSource_AssignDevice);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_InputSource_AssignDevice);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_InputSource_DeviceId _DeviceId;

public:
    CK_PROPERTY_GET(_DeviceId);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_InputSource_AssignDevice, _DeviceId);
};

// --------------------------------------------------------------------------------------------------------------------
