#pragma once

#include "CkProceduralAnimation/CkProceduralAnimation_Fragment_Data.h"
#include "CkProceduralAnimation/SurfaceMotion/CkSurfaceMotion_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Request/CkRequest_Completion.h"

#include "CkEcsExt/CkEcsExt_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

#include "CkSurfaceMotion_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_SurfaceMotion"))
class CKPROCEDURALANIMATION_API UCk_Utils_SurfaceMotion_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_SurfaceMotion_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_SurfaceMotion);

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|SurfaceMotion",
              DisplayName="[Ck][SurfaceMotion] Add")
    static FCk_Handle_SurfaceMotion
    Add(
        UPARAM(ref) FCk_Handle_Transform& InBody,
        const FCk_SurfaceMotion_Spec& InParams);

public:
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|SurfaceMotion",
        DisplayName="[Ck][SurfaceMotion] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_SurfaceMotion
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|SurfaceMotion",
        DisplayName="[Ck][SurfaceMotion] Handle -> SurfaceMotion Handle",
        meta = (CompactNodeTitle = "<AsSurfaceMotion>", BlueprintAutocast))
    static FCk_Handle_SurfaceMotion
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName="[Ck] Get Invalid SurfaceMotion Handle",
        Category = "Ck|Utils|SurfaceMotion",
        meta = (CompactNodeTitle = "INVALID_SurfaceMotionHandle", Keywords = "make"))
    static FCk_Handle_SurfaceMotion
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|SurfaceMotion",
              DisplayName="[Ck][SurfaceMotion] Get Status")
    static ECk_ProceduralAnimation_Status
    Get_Status(
        const FCk_Handle_SurfaceMotion& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|SurfaceMotion",
              DisplayName="[Ck][SurfaceMotion] Get Failure")
    static ECk_SurfaceMotion_Failure
    Get_Failure(
        const FCk_Handle_SurfaceMotion& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|SurfaceMotion",
              DisplayName="[Ck][SurfaceMotion] Get Support")
    static ECk_SurfaceMotion_Support
    Get_Support(
        const FCk_Handle_SurfaceMotion& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|SurfaceMotion",
              DisplayName="[Ck][SurfaceMotion] Get Velocity")
    static FVector
    Get_Velocity(
        const FCk_Handle_SurfaceMotion& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|SurfaceMotion",
              DisplayName="[Ck][SurfaceMotion] Get Support Normal")
    static FVector
    Get_SupportNormal(
        const FCk_Handle_SurfaceMotion& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|SurfaceMotion",
              DisplayName="[Ck][SurfaceMotion] Get Contact Query")
    static ECk_SurfaceMotion_ContactQuery
    Get_ContactQuery(
        const FCk_Handle_SurfaceMotion& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|SurfaceMotion",
              DisplayName="[Ck][SurfaceMotion] Get Contact Source")
    static ECk_SurfaceMotion_ContactSource
    Get_ContactSource(
        const FCk_Handle_SurfaceMotion& InHandle);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|SurfaceMotion",
              DisplayName="[Ck][SurfaceMotion] Request Steering",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_SurfaceMotion
    Request_Steering(
        UPARAM(ref) FCk_Handle_SurfaceMotion& InHandle,
        FCk_Request_SurfaceMotion_Steering InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
