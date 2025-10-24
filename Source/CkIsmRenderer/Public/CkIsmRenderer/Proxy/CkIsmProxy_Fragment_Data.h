#pragma once

#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkGraphics/CkGraphics_Common.h"

#include "CkIsmProxy_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_IsmRenderer_Data;

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKISMRENDERER_API FCk_Handle_IsmProxy : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_IsmProxy); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_IsmProxy);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKISMRENDERER_API FCk_Fragment_IsmProxy_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_IsmProxy_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UCk_IsmRenderer_Data> _IsmRenderer;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _StartingState = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _LocalLocationOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FRotator _LocalRotationOffset = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _ScaleMultiplier = FVector::OneVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, TitleProperty = "Index #{_DataIndex}: {_Value}"))
    TArray<FCk_CustomPrimitiveData> _CustomInstanceDataDefaults;

public:
    CK_PROPERTY_GET(_IsmRenderer);
    CK_PROPERTY(_StartingState);
    CK_PROPERTY(_LocalLocationOffset);
    CK_PROPERTY(_LocalRotationOffset);
    CK_PROPERTY(_ScaleMultiplier);
    CK_PROPERTY(_CustomInstanceDataDefaults);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_IsmProxy_ParamsData, _IsmRenderer);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKISMRENDERER_API FCk_Request_IsmProxy_SetCustomInstanceData : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_IsmProxy_SetCustomInstanceData);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_IsmProxy_SetCustomInstanceData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<float> _CustomData;

public:
    CK_PROPERTY_GET(_CustomData);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_IsmProxy_SetCustomInstanceData, _CustomData);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKISMRENDERER_API FCk_Request_IsmProxy_SetCustomInstanceDataValue : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_IsmProxy_SetCustomInstanceDataValue);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_IsmProxy_SetCustomInstanceDataValue);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, UIMin = 0, ClampMin = 0))
    int32 _CustomDataIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _CustomDataValue = 0.0f;

public:
    CK_PROPERTY_GET(_CustomDataIndex);
    CK_PROPERTY_GET(_CustomDataValue);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_IsmProxy_SetCustomInstanceDataValue, _CustomDataIndex, _CustomDataValue);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKISMRENDERER_API FCk_Request_IsmProxy_SetCustomPrimitiveData : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_IsmProxy_SetCustomPrimitiveData);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_IsmProxy_SetCustomPrimitiveData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_CustomPrimitiveData _Data;

public:
    CK_PROPERTY_GET(_Data);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_IsmProxy_SetCustomPrimitiveData, _Data);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKISMRENDERER_API FCk_Request_IsmProxy_EnableDisable : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_IsmProxy_EnableDisable);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_IsmProxy_EnableDisable);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _EnableDisable = ECk_EnableDisable::Disable;

public:
    CK_PROPERTY_GET(_EnableDisable);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_IsmProxy_EnableDisable, _EnableDisable);
};

// --------------------------------------------------------------------------------------------------------------------
