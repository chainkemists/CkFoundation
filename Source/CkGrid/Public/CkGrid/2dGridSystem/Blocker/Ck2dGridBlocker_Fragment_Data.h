#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Fragment_Data.h"

#include "Ck2dGridBlocker_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_2dGridBlocker_Requests;
}

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKGRID_API FCk_Handle_2dGridBlocker : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_2dGridBlocker); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_2dGridBlocker);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGRID_API FCk_Fragment_2dGridBlocker_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_2dGridBlocker_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Handle_2dGridSystem _Grid;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FIntPoint _RangeMin = FIntPoint::ZeroValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FIntPoint _RangeMax = FIntPoint::ZeroValue;

public:
    CK_PROPERTY(_Grid);
    CK_PROPERTY(_RangeMin);
    CK_PROPERTY(_RangeMax);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_2dGridBlocker_ParamsData, _Grid, _RangeMin, _RangeMax);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGRID_API FCk_Request_2dGridBlocker_SetActive : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    friend class ck::FProcessor_2dGridBlocker_Requests;

public:
    CK_GENERATED_BODY(FCk_Request_2dGridBlocker_SetActive);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_2dGridBlocker_SetActive);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    bool _Active = true;

public:
    CK_PROPERTY(_Active);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_2dGridBlocker_SetActive, _Active);
};

// --------------------------------------------------------------------------------------------------------------------
