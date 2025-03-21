#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkUI/UserWidget/CkUserWidget.h"

#include "CkWorldSpaceWidget_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKUI_API FCk_Handle_WorldSpaceWidget : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_WorldSpaceWidget); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_WorldSpaceWidget);

//--------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKUI_API FCk_Fragment_WorldSpaceWidget_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_WorldSpaceWidget_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    TWeakObjectPtr<UCk_UserWidget_UE> _Widget;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    FVector _WorldSpaceOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    FVector2D _ScreenSpaceOffset = FVector2D::ZeroVector;

    // TODO: Add ClampToViewport settings
    // TODO: Add Occlusion settings
    // TODO: Add scaling by distance settings

public:
    CK_PROPERTY_GET(_Widget);
    CK_PROPERTY_GET(_WorldSpaceOffset);
    CK_PROPERTY_GET(_ScreenSpaceOffset);
};

// --------------------------------------------------------------------------------------------------------------------