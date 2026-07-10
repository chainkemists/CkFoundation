#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_Typesafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkVat_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_VatCollection_Data;

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Vat_LoopMode : uint8
{
    Loop,
    Once
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Vat_LoopMode);

// --------------------------------------------------------------------------------------------------------------------

// Crowd-variety knob: RandomPerInstance offsets each instance's playback phase so identical clips don't
// tick in lock-step across a crowd.
UENUM(BlueprintType)
enum class ECk_Vat_PhaseOffset : uint8
{
    None,
    RandomPerInstance
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Vat_PhaseOffset);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKVAT_API FCk_Handle_Vat : public FCk_Handle_TypeSafe { GENERATED_BODY()  CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Vat); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Vat);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKVAT_API FCk_Fragment_Vat_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Vat_ParamsData);

private:
    // Must be loaded AND baked before Add — mirror CkIskmRenderer's contract: callers async-load soft
    // references themselves and compose once resident.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UCk_VatCollection_Data> _Collection;

    // Clip to start playing on setup. None = hold the reference pose (texture row 0).
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FName _InitialClipName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _InitialPlayRate = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Vat_LoopMode _InitialLoopMode = ECk_Vat_LoopMode::Loop;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Vat_PhaseOffset _PhaseOffset = ECk_Vat_PhaseOffset::None;

public:
    CK_PROPERTY_GET(_Collection);
    CK_PROPERTY(_InitialClipName);
    CK_PROPERTY(_InitialPlayRate);
    CK_PROPERTY(_InitialLoopMode);
    CK_PROPERTY(_PhaseOffset);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_Vat_ParamsData, _Collection);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKVAT_API FCk_Request_Vat_PlayClip : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Vat_PlayClip);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Vat_PlayClip);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FName _ClipName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _PlayRate = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Vat_LoopMode _LoopMode = ECk_Vat_LoopMode::Loop;

    // Crossfade from the previous clip. Zero = instant switch.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Time _TransitionDuration;

public:
    CK_PROPERTY_GET(_ClipName);
    CK_PROPERTY(_PlayRate);
    CK_PROPERTY(_LoopMode);
    CK_PROPERTY(_TransitionDuration);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Vat_PlayClip, _ClipName);
};

// --------------------------------------------------------------------------------------------------------------------

// Freezes playback on the current frame (play rate 0, position captured). Resume by issuing PlayClip.
USTRUCT(BlueprintType)
struct CKVAT_API FCk_Request_Vat_Stop : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Vat_Stop);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Vat_Stop);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKVAT_API FCk_Request_Vat_SetPlayRate : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Vat_SetPlayRate);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Vat_SetPlayRate);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _PlayRate = 1.0f;

public:
    CK_PROPERTY_GET(_PlayRate);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Vat_SetPlayRate, _PlayRate);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Vat_OnClipFinished,
    FCk_Handle_Vat, InHandle,
    FName, InClipName);

// --------------------------------------------------------------------------------------------------------------------
