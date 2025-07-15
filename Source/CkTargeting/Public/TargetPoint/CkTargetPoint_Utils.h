#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Enums/CkEnums.h"

#include "CkEcs/Transform/CkTransform_Fragment_Data.h"

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
        const FVector& InLocation,
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
              DisplayName="[Ck][TargetPoint] Create New TargetPoint (Transient | FromLocation)",
              meta = (WorldContext="InWorldContextObject", DefaultToSelf="InWorldContextObject"))
    static FCk_Handle_Transform
    Create_Transient_FromLocation(
        const FVector& InLocation,
        const UObject* InWorldContextObject,
        ECk_Lifetime InLifetime = ECk_Lifetime::UntilDestroyed);
};

// --------------------------------------------------------------------------------------------------------------------
