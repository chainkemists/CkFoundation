#pragma once

#include "CkFormat.h"

#include <Field/FieldSystemComponent.h>

#include "CkDestructibleAnchor.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_FieldShape : uint8
{
    Box,
    Sphere,
    Plane
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_FieldShape);

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (BlueprintSpawnableComponent), MinimalAPI)
class UCk_DestructibleAnchorField_ActorComponent_UE : public UFieldSystemComponent
{
    GENERATED_BODY()

public:
    UCk_DestructibleAnchorField_ActorComponent_UE();
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (BlueprintSpawnableComponent), MinimalAPI)
class UCk_DestructibleAnchor_ActorComponent_UE : public UStaticMeshComponent
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_DestructibleAnchor_ActorComponent_UE);

public:
    UCk_DestructibleAnchor_ActorComponent_UE();

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        meta=(AllowPrivateAccess))
    ECk_FieldShape _FieldShape = ECk_FieldShape::Box;

public:
    CK_PROPERTY_GET(_FieldShape);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(MinimalAPI)
class UCk_UniformKinematic : public UUniformInteger
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_UniformKinematic);

public:
    UCk_UniformKinematic();
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(MinimalAPI)
class UCk_BoxFalloff : public UBoxFalloff
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_BoxFalloff);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(MinimalAPI)
class UCk_CullingField : public UCullingField
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_CullingField);
};

// --------------------------------------------------------------------------------------------------------------------
