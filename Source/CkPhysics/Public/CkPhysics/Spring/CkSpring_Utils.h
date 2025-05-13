#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"

#include <Kismet/KismetMathLibrary.h>

#include "CkSpring_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKPHYSICS_API UCk_Utils_Spring_Vec3_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Spring_Vec3_UE);

public:
    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get Is Spring In Motion (Vec3)",
              Category = "Ck|Utils|Physics|Spring")
    static bool
    Get_IsInMotion(
        const FVectorSpringState& InSpringState);

private:
    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Break Spring State (Vec3)",
              Category = "Ck|Utils|Physics|Spring")
    static void
    Break_SpringState(
        const FVectorSpringState& InSpringState,
        FVector& OutPrevTarget,
        FVector& OutVelocity,
        ECk_ValidInvalid& OutPrevTargetValidity);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKPHYSICS_API UCk_Utils_Spring_Float_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Spring_Float_UE);

public:
    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get Is Spring In Motion (Float)",
              Category = "Ck|Utils|Physics|Spring")
    static bool
    Get_IsInMotion(
        const FFloatSpringState& InSpringState);

private:
    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Break Spring State (Float)",
              Category = "Ck|Utils|Physics|Spring")
    static void
    Break_SpringState(
        const FFloatSpringState& InSpringState,
        float& OutPrevTarget,
        float& OutVelocity,
        ECk_ValidInvalid& OutPrevTargetValidity);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKPHYSICS_API UCk_Utils_Spring_Quat_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Spring_Quat_UE);

public:
    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get Is Spring In Motion (Quat)",
              Category = "Ck|Utils|Physics|Spring")
    static bool
    Get_IsInMotion(
        const FQuaternionSpringState& InSpringState);

private:
    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Break Spring State (Quat)",
              Category = "Ck|Utils|Physics|Spring")
    static void
    Break_SpringState(
        const FQuaternionSpringState& InSpringState,
        FQuat& OutPrevTarget,
        FVector& OutAngularVelocity,
        ECk_ValidInvalid& OutPrevTargetValidity);
};

// --------------------------------------------------------------------------------------------------------------------
