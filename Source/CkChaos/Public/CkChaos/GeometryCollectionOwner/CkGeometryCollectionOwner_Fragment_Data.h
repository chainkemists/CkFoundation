#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Types/DataAsset/CkDataAsset.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include <CkChaos/GeometryCollection/CkGeometryCollection_Fragment_Data.h>

#include "CkGeometryCollectionOwner_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKCHAOS_API FCk_Handle_GeometryCollectionOwner : public FCk_Handle_TypeSafe { GENERATED_BODY()  CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_GeometryCollectionOwner); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_GeometryCollectionOwner);

// --------------------------------------------------------------------------------------------------------------------

class UGeometryCollectionComponent;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class CKCHAOS_API UCk_Request_GeometryCollectionOwner_ApplyRadialStrain_PDA : public UCk_DataAsset_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Request_GeometryCollectionOwner_ApplyRadialStrain_PDA);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess=true, UIMin = "0.0", ClampMin = "0.0"))
    float _Radius = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess=true, UIMin = "0.0", ClampMin = "0.0"))
    float _LinearSpeed = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess=true, UIMin = "0.0", ClampMin = "0.0"))
    float _AngularSpeed = 0.0f;

    // TODO: drive the strains using a curve
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess=true, UIMin = "0.0", ClampMin = "0.0"))
    float _InternalStrain = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess=true, UIMin = "0.0", ClampMin = "0.0"))
    float _ExternalStrain = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess=true))
    ECk_GeometryCollection_ObjectState _ChangeParticleStateTo = ECk_GeometryCollection_ObjectState::NoChange;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess=true))
    FRuntimeFloatCurve _NormalizedFalloffCurve = {};

public:
    CK_PROPERTY_GET(_Radius);
    CK_PROPERTY_GET(_LinearSpeed);
    CK_PROPERTY_GET(_AngularSpeed);
    CK_PROPERTY_GET(_InternalStrain);
    CK_PROPERTY_GET(_ExternalStrain);
    CK_PROPERTY_GET(_ChangeParticleStateTo);
    CK_PROPERTY_GET(_NormalizedFalloffCurve);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCHAOS_API FCk_Request_GeometryCollectionOwner_ApplyRadialStrain_Replicated : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_GeometryCollectionOwner_ApplyRadialStrain_Replicated);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_GeometryCollectionOwner_ApplyRadialStrain_Replicated);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess=true))
    FVector _Location = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess=true))
    TWeakObjectPtr<UCk_Request_GeometryCollectionOwner_ApplyRadialStrain_PDA> _Request;

public:
    CK_PROPERTY_GET(_Location);
    CK_PROPERTY_GET(_Request);

    CK_DEFINE_CONSTRUCTORS(FCk_Request_GeometryCollectionOwner_ApplyRadialStrain_Replicated, _Location, _Request);
};

// --------------------------------------------------------------------------------------------------------------------