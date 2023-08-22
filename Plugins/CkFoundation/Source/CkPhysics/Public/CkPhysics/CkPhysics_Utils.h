#pragma once

#include "CkCore/Enums/CkEnums.h"

#include "CkPhysics/CkPhysics_Common.h"

#include "CkPhysics_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKPHYSICS_API UCk_Physics_Utils_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Physics_Utils_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Physics",
              meta = (DefaultToSelf = "InContext", HidePin = "InContext"))
    static void
    Request_SetGenerateOverlapEvents(
        UPrimitiveComponent* InComp,
        ECk_EnableDisable InEnableDisable,
        const UObject* InContext = nullptr);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Physics",
              meta = (DefaultToSelf = "InContext", HidePin = "InContext"))
    static void
    Request_SetCollisionDetectionType(
        UPrimitiveComponent* InComp,
        ECk_CollisionDetectionType InCollisionType,
        const UObject* InContext = nullptr);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Physics",
              meta = (DefaultToSelf = "InContext", HidePin = "InContext"))
    static void
    Request_SetNavigationEffects(
        UPrimitiveComponent* InComp,
        ECk_NavigationEffect InNavigationEffect,
        const UObject* InContext = nullptr);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Physics",
              meta = (DefaultToSelf = "InContext", HidePin = "InContext"))
    static void
    Request_SetOverlapBehavior(
        UPrimitiveComponent* InComp,
        ECk_ComponentOverlapBehavior InOverlapBehavior,
        const UObject* InContext = nullptr);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Physics",
              meta = (DefaultToSelf = "InContext", HidePin = "InContext"))
    static void
    Request_SetCollisionProfileName(
        UPrimitiveComponent* InComp,
        FName                InCollisionProfileName,
        const UObject*       InContext = nullptr);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Physics",
              meta = (DefaultToSelf = "InContext", HidePin = "InContext"))
    static void
    Request_SetCollisionEnabled(
        UPrimitiveComponent* InComp,
        TEnumAsByte<ECollisionEnabled::Type> InCollisionEnabled,
        const UObject* InContext = nullptr);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Physics",
              meta = (DefaultToSelf = "InContext", HidePin = "InContext"))
    static bool
    Get_IsValidCollisionProfileName(
        FName InCollisionProfileName,
        const UObject* InContext = nullptr);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Physics",
              meta = (DefaultToSelf = "InContext", HidePin = "InContext"))
    static FName
    Get_CollisionProfileName(
        const UPrimitiveComponent* InComp,
        const UObject* InContext = nullptr);

private:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Physics")
    static FCk_ShapeDimensions
    Make_BoxShapeDimensions(const FCk_BoxExtents& InBoxExtents);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Physics")
    static FCk_ShapeDimensions
    Make_SphereShapeDimensions(const FCk_SphereRadius& InSphereRadius);

        UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Physics")
    static FCk_ShapeDimensions
    Make_CapsuleShapeDimensions(const FCk_CapsuleSize& InCapsuleSize);
};

// --------------------------------------------------------------------------------------------------------------------
