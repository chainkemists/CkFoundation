#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkGeometryCollection_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKCHAOS_API FCk_Handle_GeometryCollection : public FCk_Handle_TypeSafe { GENERATED_BODY()  CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_GeometryCollection); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_GeometryCollection);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKCHAOS_API FCk_Handle_GeometryCollectionOwner : public FCk_Handle_TypeSafe { GENERATED_BODY()  CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_GeometryCollectionOwner); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_GeometryCollectionOwner);

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
    TWeakObjectPtr<UGeometryCollectionComponent> _GeometryCollection;

public:
    CK_PROPERTY_GET(_GeometryCollection);

    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_GeometryCollection_ParamsData, _GeometryCollection);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCHAOS_API FCk_Request_GeometryCollection_ApplyStrain : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_GeometryCollection_ApplyStrain);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess=true))
    FVector _Location;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess=true))
    float _Radius = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess=true))
    FVector _LinearVelocity = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess=true))
    FVector _AngularVelocity = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess=true))
    float _InternalStrain = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess=true))
    float _ExternalStrain = 0.0f;

public:
    CK_PROPERTY_GET(_Location);
    CK_PROPERTY_GET(_Radius);
    CK_PROPERTY(_LinearVelocity);
    CK_PROPERTY(_AngularVelocity);
    CK_PROPERTY(_InternalStrain);
    CK_PROPERTY(_ExternalStrain);

    CK_DEFINE_CONSTRUCTORS(FCk_Request_GeometryCollection_ApplyStrain, _Location, _Radius);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCHAOS_API FCk_Request_GeometryCollection_ApplyAoE : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_GeometryCollection_ApplyAoE);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess=true))
    FVector _Location;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess=true))
    float _Radius = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess=true))
    FVector _LinearSpeed = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess=true))
    FVector _AngularSpeed = FVector::ZeroVector;

    // TODO: drive the strains using a curve
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess=true))
    float _InternalStrain = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess=true))
    float _ExternalStrain = 0.0f;

public:
    CK_PROPERTY_GET(_Location);
    CK_PROPERTY_GET(_Radius);
    CK_PROPERTY(_LinearSpeed);
    CK_PROPERTY(_AngularSpeed);
    CK_PROPERTY(_InternalStrain);
    CK_PROPERTY(_ExternalStrain);

    CK_DEFINE_CONSTRUCTORS(FCk_Request_GeometryCollection_ApplyAoE, _Location, _Radius);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class CKCHAOS_API UCk_Request_GeometryCollection_ApplyStrain_PDA : public UCk_DataAsset_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Request_GeometryCollection_ApplyStrain_PDA);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess=true))
    float _Radius = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess=true))
    FVector _LinearVelocity = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess=true))
    FVector _AngularVelocity = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess=true))
    float _InternalStrain = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess=true))
    float _ExternalStrain = 0.0f;

public:
    CK_PROPERTY_GET(_Radius);
    CK_PROPERTY_GET(_LinearVelocity);
    CK_PROPERTY_GET(_AngularVelocity);
    CK_PROPERTY_GET(_InternalStrain);
    CK_PROPERTY_GET(_ExternalStrain);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class CKCHAOS_API UCk_Request_GeometryCollection_ApplyAoE_PDA : public UCk_DataAsset_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Request_GeometryCollection_ApplyAoE_PDA);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess=true))
    float _Radius = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess=true))
    FVector _LinearSpeed = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess=true))
    FVector _AngularSpeed = FVector::ZeroVector;

    // TODO: drive the strains using a curve
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess=true))
    float _InternalStrain = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess=true))
    float _ExternalStrain = 0.0f;

public:
    CK_PROPERTY_GET(_Radius);
    CK_PROPERTY_GET(_LinearSpeed);
    CK_PROPERTY_GET(_AngularSpeed);
    CK_PROPERTY_GET(_InternalStrain);
    CK_PROPERTY_GET(_ExternalStrain);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCHAOS_API FCk_Request_GeometryCollection_ApplyStrain_Replicated : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_GeometryCollection_ApplyStrain_Replicated);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess=true))
    FVector _Location;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess=true))
    TWeakObjectPtr<UCk_Request_GeometryCollection_ApplyStrain_PDA> _Request;

public:
    CK_PROPERTY_GET(_Location);
    CK_PROPERTY_GET(_Request);

    CK_DEFINE_CONSTRUCTORS(FCk_Request_GeometryCollection_ApplyStrain_Replicated, _Location, _Request);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCHAOS_API FCk_Request_GeometryCollection_ApplyAoE_Replicated : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_GeometryCollection_ApplyAoE_Replicated);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess=true))
    FVector _Location;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess=true))
    TWeakObjectPtr<UCk_Request_GeometryCollection_ApplyAoE_PDA> _Request;

public:
    CK_PROPERTY_GET(_Location);
    CK_PROPERTY_GET(_Request);

    CK_DEFINE_CONSTRUCTORS(FCk_Request_GeometryCollection_ApplyAoE_Replicated, _Location, _Request);
};

// --------------------------------------------------------------------------------------------------------------------
