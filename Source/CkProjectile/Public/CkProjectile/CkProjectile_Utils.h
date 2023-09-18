#pragma once

#include "CkProjectile_Fragment_Params.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkNet/CkNet_Utils.h"

#include "CkProjectile_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKPROJECTILE_API UCk_Utils_Projectile_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Projectile_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Projectile",
              DisplayName="Add Projectile")
    static void
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_Projectile_ParamsData& InParams);
};

// --------------------------------------------------------------------------------------------------------------------
