#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkAntSquad_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UNiagaraSystem;
class UInstancedStaticMeshComponent;

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKANTBRIDGE_API FCk_Handle_AntSquad : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_AntSquad); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_AntSquad);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKANTBRIDGE_API FCk_Fragment_AntSquad_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_AntSquad_ParamsData);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKANTBRIDGE_API FCk_Request_AntSquad_AddAgent
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_AntSquad_AddAgent);

private:
    // Location relative to the AntSquad Entity
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    FVector _RelativeLocation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true, MinClamp = 0.0f, UIMin = 0.0f))
    float _Radius = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true, MinClamp = 0.0f, UIMin = 0.0f))
    float _Height = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true, MinClamp = 0.0f, UIMin = 0.0f))
    float _FaceAngle = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true, MinClamp = 0.0f, UIMin = 0.0f))
    FCk_Handle _AgentData;

public:
    CK_PROPERTY(_RelativeLocation);
    CK_PROPERTY(_Radius);
    CK_PROPERTY(_Height);
    CK_PROPERTY(_FaceAngle);
    CK_PROPERTY(_AgentData);
};

// --------------------------------------------------------------------------------------------------------------------