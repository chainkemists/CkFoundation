#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Request/CkRequest_Completion.h"

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkJolt/Body/CkJoltBody_Fragment_Data.h"
#include "CkJolt/Constraint/CkJoltConstraint_Fragment.h"
#include "CkJolt/Constraint/CkJoltConstraint_Fragment_Data.h"

#include "CkJoltConstraint_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_JoltConstraint"))
class CKJOLT_API UCk_Utils_JoltConstraint_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_JoltConstraint_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_JoltConstraint);

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    // Creates a constraint between body A and Params._OtherBody (invalid = the world), hosted on a NEW
    // child entity of body A — the constraint cascade-dies with A, and dies on its own when EITHER body
    // dies (liveness reap). Place both bodies where they should be constrained BEFORE calling: anchors
    // are world-space at creation.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|JoltConstraint",
              DisplayName="[Ck][JoltConstraint] Create Constraint")
    static FCk_Handle_JoltConstraint
    Create(
        UPARAM(ref) FCk_Handle_JoltBody& InBodyA,
        const FCk_Fragment_JoltConstraint_ParamsData& InParams);

public:
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|JoltConstraint",
        DisplayName="[Ck][JoltConstraint] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_JoltConstraint
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|JoltConstraint",
        DisplayName="[Ck][JoltConstraint] Handle -> JoltConstraint Handle",
        meta = (CompactNodeTitle = "<AsJoltConstraint>", BlueprintAutocast))
    static FCk_Handle_JoltConstraint
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid JoltConstraint Handle",
        Category = "Ck|Utils|JoltConstraint",
        meta = (CompactNodeTitle = "INVALID_JoltConstraintHandle", Keywords = "make"))
    static FCk_Handle_JoltConstraint
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|JoltConstraint",
              DisplayName="[Ck][JoltConstraint] Get Constraint Type")
    static ECk_JoltConstraint_Type
    Get_ConstraintType(
        const FCk_Handle_JoltConstraint& InConstraint);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|JoltConstraint",
              DisplayName="[Ck][JoltConstraint] Get Is Constraint Added")
    static bool
    Get_IsConstraintAdded(
        const FCk_Handle_JoltConstraint& InConstraint);

    // Hinge only: the current rotation angle in degrees (0 = the creation pose). 0 on any other type
    // (with an ensure) and while the constraint is not yet created.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|JoltConstraint",
              DisplayName="[Ck][JoltConstraint] Get Hinge Current Angle (Degrees)")
    static float
    Get_Hinge_CurrentAngleDegrees(
        const FCk_Handle_JoltConstraint& InConstraint);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|JoltConstraint",
              DisplayName="[Ck][JoltConstraint] Request Set Enabled",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_JoltConstraint
    Request_SetEnabled(
        UPARAM(ref) FCk_Handle_JoltConstraint& InConstraint,
        const FCk_Request_JoltConstraint_SetEnabled& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|JoltConstraint",
              DisplayName="[Ck][JoltConstraint] Request Set Distance Range",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_JoltConstraint
    Request_Distance_SetRange(
        UPARAM(ref) FCk_Handle_JoltConstraint& InConstraint,
        const FCk_Request_JoltConstraint_Distance_SetRange& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|JoltConstraint",
              DisplayName="[Ck][JoltConstraint] Request Set Hinge Motor",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_JoltConstraint
    Request_Hinge_SetMotor(
        UPARAM(ref) FCk_Handle_JoltConstraint& InConstraint,
        const FCk_Request_JoltConstraint_Hinge_SetMotor& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
