#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_Typesafe.h"

#include "CkPlayer_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKRELATIONSHIP_API FCk_Handle_Player : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Player); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Player);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Player_ID : uint8
{
    Zero = 0,
    One,
    Two,
    Three,
    Four,
    Five,
    Six,
    Seven,
    Eight,

    Unassigned
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Player_ID);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_PlayerChanged,
    FCk_Handle_Player, InHandle,
    ECk_Player_ID, InOldID,
    ECk_Player_ID, InNewID);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FCk_Delegate_PlayerChanged_MC,
    FCk_Handle_Player, InHandle,
    ECk_Player_ID, InOldID,
    ECk_Player_ID, InNewID);

// --------------------------------------------------------------------------------------------------------------------