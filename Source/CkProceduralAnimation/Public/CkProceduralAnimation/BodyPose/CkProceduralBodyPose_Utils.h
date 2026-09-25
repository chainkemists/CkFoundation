#pragma once

#include "CkProceduralAnimation/BodyPose/CkProceduralBodyPose_Fragment_Data.h"
#include "CkProceduralAnimation/CkProceduralAnimation_Fragment_Data.h"
#include "CkProceduralAnimation/Gait/CkProceduralGait_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcsExt/CkEcsExt_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

#include "CkProceduralBodyPose_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_ProceduralBodyPose"))
class CKPROCEDURALANIMATION_API UCk_Utils_ProceduralBodyPose_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ProceduralBodyPose_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_ProceduralBodyPose);

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ProceduralBodyPose",
              DisplayName="[Ck][ProceduralBodyPose] Add")
    static FCk_Handle_ProceduralBodyPose
    Add(
        UPARAM(ref) FCk_Handle_ProceduralGait& InGait,
        const FCk_ProceduralBodyPose_Spec& InParams);

public:
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ProceduralBodyPose",
        DisplayName="[Ck][ProceduralBodyPose] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_ProceduralBodyPose
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|ProceduralBodyPose",
        DisplayName="[Ck][ProceduralBodyPose] Handle -> ProceduralBodyPose Handle",
        meta = (CompactNodeTitle = "<AsProceduralBodyPose>", BlueprintAutocast))
    static FCk_Handle_ProceduralBodyPose
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName="[Ck] Get Invalid ProceduralBodyPose Handle",
        Category = "Ck|Utils|ProceduralBodyPose",
        meta = (CompactNodeTitle = "INVALID_ProceduralBodyPoseHandle", Keywords = "make"))
    static FCk_Handle_ProceduralBodyPose
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ProceduralBodyPose",
              DisplayName="[Ck][ProceduralBodyPose] Get Status")
    static ECk_ProceduralAnimation_Status
    Get_Status(
        const FCk_Handle_ProceduralBodyPose& InBodyPose);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ProceduralBodyPose",
              DisplayName="[Ck][ProceduralBodyPose] Get Failure")
    static ECk_ProceduralBodyPose_Failure
    Get_Failure(
        const FCk_Handle_ProceduralBodyPose& InBodyPose);

    /** Body-local; the presentation entity is posed at Offset * Body. Identity unless the body pose is Ready. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ProceduralBodyPose",
              DisplayName="[Ck][ProceduralBodyPose] Get Offset")
    static FTransform
    Get_Offset(
        const FCk_Handle_ProceduralBodyPose& InBodyPose);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ProceduralBodyPose",
              DisplayName="[Ck][ProceduralBodyPose] Get Presentation")
    static FCk_Handle_Transform
    Get_Presentation(
        const FCk_Handle_ProceduralBodyPose& InBodyPose);
};

// --------------------------------------------------------------------------------------------------------------------
