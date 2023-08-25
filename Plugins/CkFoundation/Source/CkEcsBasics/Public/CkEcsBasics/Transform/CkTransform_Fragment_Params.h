#pragma once

#include "CkCore/Enums/CkEnums.h"

#include "CkMacros/CkMacros.h"

#include "CkTransform_Fragment_Params.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKECSBASICS_API FCk_Request_Transform_SetLocation
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Transform_SetLocation);

public:
    FCk_Request_Transform_SetLocation() = default;
    FCk_Request_Transform_SetLocation(
        FVector              InNewLocation,
        ECk_RelativeAbsolute InRelativeAbsolute);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _NewLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_RelativeAbsolute _RelativeAbsolute = ECk_RelativeAbsolute::Absolute;

public:
    CK_PROPERTY(_NewLocation)
    CK_PROPERTY(_RelativeAbsolute)
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKECSBASICS_API FCk_Request_Transform_AddLocationOffset
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Transform_AddLocationOffset);

public:
    FCk_Request_Transform_AddLocationOffset() = default;
    FCk_Request_Transform_AddLocationOffset(
        FVector        InDeltaLocation,
        ECk_LocalWorld InLocalWorld);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _DeltaLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_LocalWorld _LocalWorld = ECk_LocalWorld::World;

public:
    CK_PROPERTY(_DeltaLocation)
    CK_PROPERTY_GET(_LocalWorld)
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKECSBASICS_API FCk_Request_Transform_SetRotation
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Transform_SetRotation);

public:
    FCk_Request_Transform_SetRotation() = default;
    FCk_Request_Transform_SetRotation(
        FRotator InNewLocation,
        ECk_RelativeAbsolute InRelativeAbsolute);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FRotator _NewRotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_RelativeAbsolute _RelativeAbsolute = ECk_RelativeAbsolute::Absolute;

public:
    CK_PROPERTY_GET(_NewRotation)
    CK_PROPERTY_GET(_RelativeAbsolute)
};

USTRUCT(BlueprintType)
struct CKECSBASICS_API FCk_Request_Transform_AddRotationOffset
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Transform_AddRotationOffset);

public:
    FCk_Request_Transform_AddRotationOffset() = default;
    FCk_Request_Transform_AddRotationOffset(
        FRotator       InDeltaRotation,
        ECk_LocalWorld InLocalWorld);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FRotator _DeltaRotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_LocalWorld _LocalWorld = ECk_LocalWorld::World;

public:
    CK_PROPERTY_GET(_DeltaRotation)
    CK_PROPERTY_GET(_LocalWorld)
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKECSBASICS_API FCk_Request_Transform_SetScale
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Transform_SetScale);

public:
    FCk_Request_Transform_SetScale() = default;
    FCk_Request_Transform_SetScale(
        FVector              InNewScale,
        ECk_RelativeAbsolute InRelativeAbsolute);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _NewScale = FVector::OneVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_RelativeAbsolute _RelativeAbsolute = ECk_RelativeAbsolute::Absolute;

public:
    CK_PROPERTY_GET(_NewScale)
    CK_PROPERTY_GET(_RelativeAbsolute)
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKECSBASICS_API FCk_Request_Transform_SetTransform
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Transform_SetTransform);

public:
    FCk_Request_Transform_SetTransform() = default;
    FCk_Request_Transform_SetTransform(
        FTransform           InNewTransform,
        ECk_RelativeAbsolute InRelativeAbsolute);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FTransform _NewTransform = FTransform::Identity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_RelativeAbsolute _RelativeAbsolute = ECk_RelativeAbsolute::Absolute;

public:
    CK_PROPERTY_GET(_NewTransform)
    CK_PROPERTY_GET(_RelativeAbsolute)
};

// --------------------------------------------------------------------------------------------------------------------

// TODO: Add AddOffset request variations
