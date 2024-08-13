#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle_Typesafe.h"

#include "CkTeam_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKRELATIONSHIP_API FCk_Handle_Team : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Team); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Team);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Team_ID : uint8
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

    Unassigned = 255
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Team_ID);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_TeamChanged,
    FCk_Handle_Team, InHandle,
    ECk_Team_ID, InOldID,
    ECk_Team_ID, InNewID);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FCk_Delegate_TeamChanged_MC,
    FCk_Handle_Team, InHandle,
    ECk_Team_ID, InOldID,
    ECk_Team_ID, InNewID);

// --------------------------------------------------------------------------------------------------------------------
