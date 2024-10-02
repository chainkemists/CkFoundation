#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkGeometryCollection_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_GeometryCollection_HandleRequests;
}

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKCHAOS_API FCk_Handle_GeometryCollection : public FCk_Handle_TypeSafe { GENERATED_BODY()  CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_GeometryCollection); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_GeometryCollection);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_GeometryCollection_ObjectState : uint8
{
    Uninitialized = 0,
    Sleeping = 1,
    Kinematic = 2,
    Static = 3,
    Dynamic = 4,

    NoChange
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_GeometryCollection_ObjectState);

// --------------------------------------------------------------------------------------------------------------------

class UGeometryCollectionComponent;

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCHAOS_API FCk_Fragment_GeometryCollection_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_GeometryCollection_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess))
    TWeakObjectPtr<UGeometryCollectionComponent> _GeometryCollection;

public:
    CK_PROPERTY_GET(_GeometryCollection);

    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_GeometryCollection_ParamsData, _GeometryCollection);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCHAOS_API FCk_Request_GeometryCollection_ApplyRadialStrain : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    friend class ck::FProcessor_GeometryCollection_HandleRequests;

public:
    CK_GENERATED_BODY(FCk_Request_GeometryCollection_ApplyRadialStrain);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess=true))
    FVector _Location;

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

private:
    float _IncrementalRadius = 0.0f;

private:
    CK_PROPERTY(_IncrementalRadius);

public:
    CK_PROPERTY_GET(_Location);
    CK_PROPERTY_GET(_Radius);
    CK_PROPERTY(_LinearSpeed);
    CK_PROPERTY(_AngularSpeed);
    CK_PROPERTY(_InternalStrain);
    CK_PROPERTY(_ExternalStrain);
    CK_PROPERTY(_ChangeParticleStateTo);
    CK_PROPERTY(_NormalizedFalloffCurve);

    CK_DEFINE_CONSTRUCTORS(FCk_Request_GeometryCollection_ApplyRadialStrain, _Location, _Radius);
};

// --------------------------------------------------------------------------------------------------------------------