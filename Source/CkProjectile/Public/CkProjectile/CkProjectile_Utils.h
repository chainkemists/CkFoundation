#pragma once

#include "CkProjectile_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"

#include "CkProjectile_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Projectile"))
class CKPROJECTILE_API UCk_Utils_Projectile_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Projectile_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Projectile);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Projectile",
              DisplayName="[Ck][Projectile] Add Feature")
    static FCk_Handle_Projectile
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_Projectile_ParamsData& InParams,
        ECk_Replication InReplicates);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Projectile",
              DisplayName="[Ck][Projectile] Create")
    static FCk_Handle_Projectile
    Create(
        UPARAM(ref) FCk_Handle& InOwner,
        const FCk_Fragment_Projectile_ParamsData& InParams,
        ECk_Replication InReplicates);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Projectile",
              DisplayName="[Ck][Projectile] Has")
    static bool
    Has(
        const FCk_Handle& InHandle);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Projectile",
              meta = (AutoCreateRefTerm = "InOptionalPayload, InDelegate, InCompletionDelegate"),
              DisplayName="[Ck][Projectile] Request Calculate Aim Ahead")
    static void
    Request_CalculateAimAhead(
        const FCk_Handle& InHandle,
        const FCk_Request_Projectile_CalculateAimAhead& InRequest,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Delegate_Projectile_OnAimAheadCalculated& InDelegate,
        const FCk_Delegate_Request_OnCompleted& InCompletionDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
