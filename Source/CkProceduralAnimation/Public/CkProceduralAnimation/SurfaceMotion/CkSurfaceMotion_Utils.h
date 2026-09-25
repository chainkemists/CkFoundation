#pragma once

#include "CkProceduralAnimation/SurfaceMotion/CkSurfaceMotion_Fragment_Data.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkSurfaceMotion_Utils.generated.h"

UCLASS(NotBlueprintable, meta = (ScriptMixin = "FCk_Handle_SurfaceMotion"))
class CKPROCEDURALANIMATION_API UCk_Utils_SurfaceMotion_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_SurfaceMotion_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_SurfaceMotion);

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|SurfaceMotion")
    static FCk_Handle_SurfaceMotion
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_SurfaceMotion_ParamsData& InParams);

    UFUNCTION(BlueprintPure, Category = "Ck|SurfaceMotion")
    static bool
    Has(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|SurfaceMotion")
    static bool
    Get_IsReady(
        const FCk_Handle_SurfaceMotion& InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|SurfaceMotion")
    static bool
    Get_IsGrounded(
        const FCk_Handle_SurfaceMotion& InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|SurfaceMotion")
    static FVector
    Get_Velocity(
        const FCk_Handle_SurfaceMotion& InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|SurfaceMotion")
    static FVector
    Get_SupportNormal(
        const FCk_Handle_SurfaceMotion& InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|SurfaceMotion")
    static bool
    Get_HasTrustedContact(
        const FCk_Handle_SurfaceMotion& InHandle);

    UFUNCTION(BlueprintCallable, Category = "Ck|SurfaceMotion", meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_SurfaceMotion
    Request_Steering(
        UPARAM(ref) FCk_Handle_SurfaceMotion& InHandle,
        FCk_Request_SurfaceMotion_Steering InRequest, const FCk_Delegate_Request_OnCompleted& InDelegate);

private:

    UFUNCTION(BlueprintCallable, Category = "Ck|SurfaceMotion", meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_SurfaceMotion
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure, Category = "Ck|SurfaceMotion", meta = (BlueprintAutocast, CompactNodeTitle = "<AsSurfaceMotion>"))
    static FCk_Handle_SurfaceMotion
    DoCastChecked(
        FCk_Handle InHandle);
};
