#pragma once

#include "CkHandle_TypeSafe.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkAntSquad_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UNiagaraSystem;
class UInstancedStaticMeshComponent;

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKANTBRIDGE_API FCk_Handle_AntSquad : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_AntSquad); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_AntSquad);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKANTBRIDGE_API FCk_Handle_AntAgent_Renderer : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_AntAgent_Renderer); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_AntAgent_Renderer);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKANTBRIDGE_API FCk_Fragment_AntAgent_Renderer_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_AntAgent_Renderer_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UStaticMesh> _Mesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _MeshScale = FVector::OneVector;

public:
    CK_PROPERTY_GET(_Mesh);
    CK_PROPERTY_GET(_MeshScale);
};

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

public:
    CK_PROPERTY(_RelativeLocation);
    CK_PROPERTY(_Radius);
    CK_PROPERTY(_Height);
    CK_PROPERTY(_FaceAngle);
};

// --------------------------------------------------------------------------------------------------------------------