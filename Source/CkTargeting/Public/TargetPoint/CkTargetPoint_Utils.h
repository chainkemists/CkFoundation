#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Enums/CkEnums.h"

#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

#include "CkTargetPoint_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKTARGETING_API UCk_Utils_TargetPoint_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_TargetPoint_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|TargetPoint",
              DisplayName="[Ck][TargetPoint] Create New TargetPoint")
    static FCk_Handle_Transform
    Create(
        const FCk_Handle& InOwner,
        const FTransform& InTransform,
        ECk_Lifetime InLifetime = ECk_Lifetime::UntilDestroyed);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|TargetPoint",
              DisplayName="[Ck][TargetPoint] Create New TargetPoint (From Location)")
    static FCk_Handle_Transform
    Create_FromLocation(
        const FCk_Handle& InOwner,
        FVector InLocation,
        ECk_Lifetime InLifetime = ECk_Lifetime::UntilDestroyed);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|TargetPoint",
              DisplayName="[Ck][TargetPoint] Create New TargetPoint (From Location & Rotation)")
    static FCk_Handle_Transform
    Create_FromLocationAndRotation(
        const FCk_Handle& InOwner,
        FVector InLocation,
        FRotator InRotation,
        ECk_Lifetime InLifetime = ECk_Lifetime::UntilDestroyed);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|TargetPoint",
              DisplayName="[Ck][TargetPoint] Create New TargetPoint (Transient)",
              meta = (WorldContext="InWorldContextObject", DefaultToSelf="InWorldContextObject"))
    static FCk_Handle_Transform
    Create_Transient(
        const FTransform& InTransform,
        const UObject* InWorldContextObject,
        ECk_Lifetime InLifetime = ECk_Lifetime::UntilDestroyed);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|TargetPoint",
              DisplayName="[Ck][TargetPoint] Create New TargetPoint (Transient | From Location)",
              meta = (WorldContext="InWorldContextObject", DefaultToSelf="InWorldContextObject"))
    static FCk_Handle_Transform
    Create_Transient_FromLocation(
        FVector InLocation,
        const UObject* InWorldContextObject,
        ECk_Lifetime InLifetime = ECk_Lifetime::UntilDestroyed);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|TargetPoint",
              DisplayName="[Ck][TargetPoint] Create New TargetPoint (Transient | From Location & Rotation)",
              meta = (WorldContext="InWorldContextObject", DefaultToSelf="InWorldContextObject"))
    static FCk_Handle_Transform
    Create_Transient_FromLocationAndRotation(
        FVector InLocation,
        FRotator InRotation,
        const UObject* InWorldContextObject,
        ECk_Lifetime InLifetime = ECk_Lifetime::UntilDestroyed);
};

// --------------------------------------------------------------------------------------------------------------------
