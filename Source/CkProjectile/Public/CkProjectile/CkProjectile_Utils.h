#pragma once

#include "CkProjectile_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Net/CkNet_Utils.h"

#include "CkProjectile_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKPROJECTILE_API UCk_Utils_Projectile_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Projectile_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][Projectile] Add Feature")
    static void
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_Projectile_ParamsData& InParams);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              meta = (AutoCreateRefTerm = "InOptionalPayload, InDelegate"),
              DisplayName="[Ck][Projectile] Request Calculate Aim Ahead")
    static void
    Request_CalculateAimAhead(
        const FCk_Handle& InHandle,
        const FCk_Request_Projectile_CalculateAimAhead& InRequest,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Delegate_Projectile_OnAimAheadCalculated& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
