#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_Typesafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkVisibleRange_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_VisibleRange_ShowHide : uint8
{
    Show,
    Hide
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_VisibleRange_ShowHide);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKVISIBLERANGE_API FCk_Handle_VisibleRange : public FCk_Handle_TypeSafe { GENERATED_BODY()  CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_VisibleRange); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_VisibleRange);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKVISIBLERANGE_API FCk_Fragment_VisibleRange_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_VisibleRange_ParamsData);

private:
    // 0 = none. World units (cm). The entity is hidden while NEARER than this range.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0))
    float _MinRange = 0.0f;

    // 0 = unlimited. World units (cm). The entity is hidden while FARTHER than this range.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0))
    float _MaxRange = 0.0f;

    // 0 = hard cut at the range boundaries. Width (cm) of the fade band INSIDE each active
    // boundary that FadeAlpha ramps 0->1 across. Boundaries that are 0/off never fade.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0))
    float _FadeBandCm = 0.0f;

    // 0 = every tick. How often the range/fade re-evaluates against the last-supplied distance.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _UpdateInterval;

public:
    CK_PROPERTY(_MinRange);
    CK_PROPERTY_GET(_MaxRange);
    CK_PROPERTY(_FadeBandCm);
    CK_PROPERTY(_UpdateInterval);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_VisibleRange_ParamsData, _MaxRange);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKVISIBLERANGE_API FCk_Request_VisibleRange_SetVisibility : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_VisibleRange_SetVisibility);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_VisibleRange_SetVisibility);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_VisibleRange_ShowHide _ShowHide = ECk_VisibleRange_ShowHide::Show;

public:
    CK_PROPERTY_GET(_ShowHide);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_VisibleRange_SetVisibility, _ShowHide);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct CKVISIBLERANGE_API FCk_Request_VisibleRange_ApplyRangeState : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_VisibleRange_ApplyRangeState);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_VisibleRange_ApplyRangeState);

private:
    UPROPERTY()
    bool _IsOutOfRange = false;

public:
    CK_PROPERTY_GET(_IsOutOfRange);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_VisibleRange_ApplyRangeState, _IsOutOfRange);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_VisibleRange_HiddenChanged,
    FCk_Handle_VisibleRange, InHandle,
    bool, InIsHidden);

// --------------------------------------------------------------------------------------------------------------------
