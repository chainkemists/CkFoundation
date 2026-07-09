#pragma once

#include "CkCore/Format/CkFormat.h"

#include "CkLoadingScreen_Common.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_LoadingScreen_Visibility : uint8
{
    Visible,
    Hidden
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_LoadingScreen_Visibility);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_LoadingScreen_OnVisibilityChanged,
    ECk_LoadingScreen_Visibility,
    InVisibility);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FCk_Delegate_LoadingScreen_OnVisibilityChanged_MC,
    ECk_LoadingScreen_Visibility,
    InVisibility);

// --------------------------------------------------------------------------------------------------------------------
