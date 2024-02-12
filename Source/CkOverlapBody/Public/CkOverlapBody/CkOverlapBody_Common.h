#pragma once

#include "CkEcs/Handle/CkHandle.h"

#include <Components/BoxComponent.h>
#include <Components/CapsuleComponent.h>
#include <Components/SphereComponent.h>

#include "CkOverlapBody_Common.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_OverlapBody_Type : uint8
{
    Other UMETA(Hidden),
    Marker,
    Sensor
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_OverlapBody_Type);

// --------------------------------------------------------------------------------------------------------------------

UINTERFACE(NotBlueprintable)
class UCk_OverlapBody_Interface : public UInterface { GENERATED_BODY() };
class CKOVERLAPBODY_API ICk_OverlapBody_Interface
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ICk_OverlapBody_Interface);

public:
    virtual auto Get_Type() const -> ECk_OverlapBody_Type
    PURE_VIRTUAL(ICk_OverlapBody_Interface::Get_Type, return ECk_OverlapBody_Type::Other; );

    virtual auto Get_OwningEntity() const -> const FCk_Handle&
    PURE_VIRTUAL(ICk_OverlapBody_Interface::Get_OwningEntity,
    {
        static FCk_Handle InvalidEntity;
        return InvalidEntity;
    });
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract,
       BlueprintType,
       NotBlueprintable,
       HideCategories("Replication", "ComponentTick", "Rendering", "Activation", "Tags", "ComponentReplication", "Mobile", "RayTracing",
                      "Collision", "AssetUserData", "Cooking", "Sockets", "Variable", "Navigation", "HLOD", "Physics"))
class CKOVERLAPBODY_API UCk_OverlapBody_ActorComponent_Box_UE
    : public UBoxComponent
    , public ICk_OverlapBody_Interface
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_OverlapBody_ActorComponent_Box_UE);

public:
    UCk_OverlapBody_ActorComponent_Box_UE();
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract,
       BlueprintType,
       NotBlueprintable,
       HideCategories("Replication", "ComponentTick", "Rendering", "Activation", "Tags", "ComponentReplication", "Mobile", "RayTracing",
                      "Collision", "AssetUserData", "Cooking", "Sockets", "Variable", "Navigation", "HLOD", "Physics"))
class CKOVERLAPBODY_API UCk_OverlapBody_ActorComponent_Sphere_UE
    : public USphereComponent
    , public ICk_OverlapBody_Interface
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_OverlapBody_ActorComponent_Sphere_UE);

public:
    UCk_OverlapBody_ActorComponent_Sphere_UE();
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract,
       BlueprintType,
       NotBlueprintable,
       HideCategories("Replication", "ComponentTick", "Rendering", "Activation", "Tags", "ComponentReplication", "Mobile", "RayTracing",
                      "Collision", "AssetUserData", "Cooking", "Sockets", "Variable", "Navigation", "HLOD", "Physics"))
class CKOVERLAPBODY_API UCk_OverlapBody_ActorComponent_Capsule_UE
    : public UCapsuleComponent
    , public ICk_OverlapBody_Interface
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_OverlapBody_ActorComponent_Capsule_UE);

public:
    UCk_OverlapBody_ActorComponent_Capsule_UE();
};

// --------------------------------------------------------------------------------------------------------------------
