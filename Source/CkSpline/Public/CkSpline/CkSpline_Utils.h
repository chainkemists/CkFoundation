#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Enums/CkEnums.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

#include "CkSpline/CkSpline_Fragment_Data.h"

#include "CkSpline_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class USplineComponent;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Spline"))
class CKSPLINE_API UCk_Utils_Spline_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Spline_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Spline);

public:
    // --------------------------------------------------------------------------------------------------------------------
    // ADD — attach the spline feature to an existing transform entity
    // --------------------------------------------------------------------------------------------------------------------

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Add Feature")
    static FCk_Handle_Spline
    Add(
        UPARAM(ref) FCk_Handle_Transform& InEntity,
        const FCk_Spline_Spec& InParams);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Add Feature (From Spline Component)")
    static FCk_Handle_Spline
    Add_FromSplineComponent(
        UPARAM(ref) FCk_Handle_Transform& InEntity,
        USplineComponent* InSourceSpline);

    // --------------------------------------------------------------------------------------------------------------------
    // CREATE — create a new transform entity and add the spline feature to it
    // --------------------------------------------------------------------------------------------------------------------

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Create New Spline")
    static FCk_Handle_Spline
    Create(
        const FCk_Handle_Transform& InOwner,
        const FCk_Spline_Spec& InParams);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Create New Spline (From Spline Component)")
    static FCk_Handle_Spline
    Create_FromSplineComponent(
        const FCk_Handle_Transform& InOwner,
        USplineComponent* InSourceSpline);

    // --------------------------------------------------------------------------------------------------------------------
    // PARAMS CONSTRUCTION
    // --------------------------------------------------------------------------------------------------------------------

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Make Params (From Points)")
    static FCk_Spline_Spec
    Make_Params_FromPoints(
        const TArray<FVector>& InLocalPoints,
        bool InClosedLoop);

    // --------------------------------------------------------------------------------------------------------------------
    // QUERIES
    // --------------------------------------------------------------------------------------------------------------------

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Get Length")
    static float
    Get_Length(
        const FCk_Handle_Spline& InSpline);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Get Number Of Points")
    static int32
    Get_NumberOfPoints(
        const FCk_Handle_Spline& InSpline);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Get Is Closed Loop")
    static bool
    Get_IsClosedLoop(
        const FCk_Handle_Spline& InSpline);

    // ---- By distance along spline ----

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Get Location At Distance")
    static FVector
    Get_LocationAtDistance(
        const FCk_Handle_Spline& InSpline,
        float InDistance);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Get Direction At Distance")
    static FVector
    Get_DirectionAtDistance(
        const FCk_Handle_Spline& InSpline,
        float InDistance);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Get Tangent At Distance")
    static FVector
    Get_TangentAtDistance(
        const FCk_Handle_Spline& InSpline,
        float InDistance);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Get Rotation At Distance")
    static FRotator
    Get_RotationAtDistance(
        const FCk_Handle_Spline& InSpline,
        float InDistance);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Get Up Vector At Distance")
    static FVector
    Get_UpVectorAtDistance(
        const FCk_Handle_Spline& InSpline,
        float InDistance);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Get Right Vector At Distance")
    static FVector
    Get_RightVectorAtDistance(
        const FCk_Handle_Spline& InSpline,
        float InDistance);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Get Scale At Distance")
    static FVector
    Get_ScaleAtDistance(
        const FCk_Handle_Spline& InSpline,
        float InDistance);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Get Transform At Distance")
    static FTransform
    Get_TransformAtDistance(
        const FCk_Handle_Spline& InSpline,
        float InDistance);

    // ---- By input key ----

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Get Location At Input Key")
    static FVector
    Get_LocationAtInputKey(
        const FCk_Handle_Spline& InSpline,
        float InInputKey);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Get Direction At Input Key")
    static FVector
    Get_DirectionAtInputKey(
        const FCk_Handle_Spline& InSpline,
        float InInputKey);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Get Rotation At Input Key")
    static FRotator
    Get_RotationAtInputKey(
        const FCk_Handle_Spline& InSpline,
        float InInputKey);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Get Transform At Input Key")
    static FTransform
    Get_TransformAtInputKey(
        const FCk_Handle_Spline& InSpline,
        float InInputKey);

    // ---- By spline point index ----

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Get Location At Point")
    static FVector
    Get_LocationAtPoint(
        const FCk_Handle_Spline& InSpline,
        int32 InPointIndex);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Get Direction At Point")
    static FVector
    Get_DirectionAtPoint(
        const FCk_Handle_Spline& InSpline,
        int32 InPointIndex);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Get Rotation At Point")
    static FRotator
    Get_RotationAtPoint(
        const FCk_Handle_Spline& InSpline,
        int32 InPointIndex);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Get Transform At Point")
    static FTransform
    Get_TransformAtPoint(
        const FCk_Handle_Spline& InSpline,
        int32 InPointIndex);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Get Input Key At Point")
    static float
    Get_InputKeyAtPoint(
        const FCk_Handle_Spline& InSpline,
        int32 InPointIndex);

    // ---- Conversions ----

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Get Input Key At Distance")
    static float
    Get_InputKeyAtDistance(
        const FCk_Handle_Spline& InSpline,
        float InDistance);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Get Distance At Input Key")
    static float
    Get_DistanceAtInputKey(
        const FCk_Handle_Spline& InSpline,
        float InInputKey);

    // ---- Meta (additional) ----

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Get Number Of Segments")
    static int32
    Get_NumberOfSegments(
        const FCk_Handle_Spline& InSpline);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Get Default Up Vector")
    static FVector
    Get_DefaultUpVector(
        const FCk_Handle_Spline& InSpline);

    // ---- By distance (additional) ----

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Get Roll At Distance")
    static float
    Get_RollAtDistance(
        const FCk_Handle_Spline& InSpline,
        float InDistance);

    // ---- By input key (additional) ----

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Get Tangent At Input Key")
    static FVector
    Get_TangentAtInputKey(
        const FCk_Handle_Spline& InSpline,
        float InInputKey);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Get Up Vector At Input Key")
    static FVector
    Get_UpVectorAtInputKey(
        const FCk_Handle_Spline& InSpline,
        float InInputKey);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Get Right Vector At Input Key")
    static FVector
    Get_RightVectorAtInputKey(
        const FCk_Handle_Spline& InSpline,
        float InInputKey);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Get Scale At Input Key")
    static FVector
    Get_ScaleAtInputKey(
        const FCk_Handle_Spline& InSpline,
        float InInputKey);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Get Roll At Input Key")
    static float
    Get_RollAtInputKey(
        const FCk_Handle_Spline& InSpline,
        float InInputKey);

    // ---- By spline point index (additional) ----

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Get Tangent At Point")
    static FVector
    Get_TangentAtPoint(
        const FCk_Handle_Spline& InSpline,
        int32 InPointIndex);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Get Arrive Tangent At Point")
    static FVector
    Get_ArriveTangentAtPoint(
        const FCk_Handle_Spline& InSpline,
        int32 InPointIndex);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Get Leave Tangent At Point")
    static FVector
    Get_LeaveTangentAtPoint(
        const FCk_Handle_Spline& InSpline,
        int32 InPointIndex);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Get Up Vector At Point")
    static FVector
    Get_UpVectorAtPoint(
        const FCk_Handle_Spline& InSpline,
        int32 InPointIndex);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Get Right Vector At Point")
    static FVector
    Get_RightVectorAtPoint(
        const FCk_Handle_Spline& InSpline,
        int32 InPointIndex);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Get Scale At Point")
    static FVector
    Get_ScaleAtPoint(
        const FCk_Handle_Spline& InSpline,
        int32 InPointIndex);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Get Roll At Point")
    static float
    Get_RollAtPoint(
        const FCk_Handle_Spline& InSpline,
        int32 InPointIndex);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Get Distance At Point")
    static float
    Get_DistanceAtPoint(
        const FCk_Handle_Spline& InSpline,
        int32 InPointIndex);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Get Point Type")
    static TEnumAsByte<ESplinePointType::Type>
    Get_PointType(
        const FCk_Handle_Spline& InSpline,
        int32 InPointIndex);

    // ---- Closest to a world-space location ----

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Find Input Key Closest To World Location")
    static float
    Find_InputKeyClosestToWorldLocation(
        const FCk_Handle_Spline& InSpline,
        FVector InWorldLocation);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Find Location Closest To World Location")
    static FVector
    Find_LocationClosestToWorldLocation(
        const FCk_Handle_Spline& InSpline,
        FVector InWorldLocation);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Find Direction Closest To World Location")
    static FVector
    Find_DirectionClosestToWorldLocation(
        const FCk_Handle_Spline& InSpline,
        FVector InWorldLocation);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Find Tangent Closest To World Location")
    static FVector
    Find_TangentClosestToWorldLocation(
        const FCk_Handle_Spline& InSpline,
        FVector InWorldLocation);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Find Rotation Closest To World Location")
    static FRotator
    Find_RotationClosestToWorldLocation(
        const FCk_Handle_Spline& InSpline,
        FVector InWorldLocation);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Find Up Vector Closest To World Location")
    static FVector
    Find_UpVectorClosestToWorldLocation(
        const FCk_Handle_Spline& InSpline,
        FVector InWorldLocation);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Find Right Vector Closest To World Location")
    static FVector
    Find_RightVectorClosestToWorldLocation(
        const FCk_Handle_Spline& InSpline,
        FVector InWorldLocation);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Find Scale Closest To World Location")
    static FVector
    Find_ScaleClosestToWorldLocation(
        const FCk_Handle_Spline& InSpline,
        FVector InWorldLocation);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Find Transform Closest To World Location")
    static FTransform
    Find_TransformClosestToWorldLocation(
        const FCk_Handle_Spline& InSpline,
        FVector InWorldLocation);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Get Distance Closest To World Location")
    static float
    Get_DistanceClosestToWorldLocation(
        const FCk_Handle_Spline& InSpline,
        FVector InWorldLocation);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Spline
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Spline",
        DisplayName = "[Ck][Spline] Handle -> Spline Handle",
        meta = (CompactNodeTitle = "<AsSpline>", BlueprintAutocast))
    static FCk_Handle_Spline
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid Spline Handle",
        Category = "Ck|Utils|Spline",
        meta = (CompactNodeTitle = "INVALID_SplineHandle", Keywords = "make"))
    static FCk_Handle_Spline
    Get_InvalidHandle() { return {}; }
};

// --------------------------------------------------------------------------------------------------------------------
