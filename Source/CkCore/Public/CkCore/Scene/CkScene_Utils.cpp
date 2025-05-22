#include "CkScene_Utils.h"

#include "CkCore/Scene/CkScene.h"
#include "CkCore/Validation/CkIsValid.h"

// ----------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Scene_UE::
    DrawPoint(
        const FCk_Handle_PrimitiveDrawInterface& InDrawHandle,
        FVector InPosition,
        FLinearColor InColor,
        float InPointSize,
        TEnumAsByte<ESceneDepthPriorityGroup> InDepthPriorityGroup)
    -> void
{
    if (ck::Is_NOT_Valid(InDrawHandle))
    { return; }


    InDrawHandle.Get_PrimitiveDrawInterface()->DrawPoint(InPosition, InColor, InPointSize, InDepthPriorityGroup);
}

auto
    UCk_Utils_Scene_UE::
    DrawLine(
        const FCk_Handle_PrimitiveDrawInterface& InDrawHandle,
        FVector InStart,
        FVector InEnd,
        FLinearColor InColor,
        TEnumAsByte<ESceneDepthPriorityGroup> InDepthPriorityGroup,
        float InThickness,
        float InDepthBias,
        bool InIsScreenSpace) -> void
{
    if (ck::Is_NOT_Valid(InDrawHandle))
    { return; }

    InDrawHandle.Get_PrimitiveDrawInterface()->DrawTranslucentLine(InStart, InEnd, InColor, InDepthPriorityGroup, InThickness, InDepthBias, InIsScreenSpace);
}

auto
    UCk_Utils_Scene_UE::
    DrawBox(
        const FCk_Handle_PrimitiveDrawInterface& InDrawHandle,
        FVector InOrigin,
        FRotator InRotation,
        FVector InExtents,
        FLinearColor InColor,
        TEnumAsByte<ESceneDepthPriorityGroup> InDepthPriorityGroup,
        float InThickness,
        float InDepthBias,
        bool InIsScreenSpace)
    -> void
{
    if (ck::Is_NOT_Valid(InDrawHandle))
    { return; }

    const auto RotMatrix = FRotationMatrix{InRotation};

    DrawOrientedWireBox(
        InDrawHandle.Get_PrimitiveDrawInterface(),
        InOrigin,
        RotMatrix.GetScaledAxis(EAxis::X),
        RotMatrix.GetScaledAxis(EAxis::Y),
        RotMatrix.GetScaledAxis(EAxis::Z),
        InExtents,
        InColor,
        InDepthPriorityGroup,
        InThickness,
        InDepthBias,
        InIsScreenSpace);
}

auto
    UCk_Utils_Scene_UE::
    DrawSphere(
        const FCk_Handle_PrimitiveDrawInterface& InDrawHandle,
        FVector InOrigin,
        float InRadius,
        int32 InNumSides,
        FLinearColor InColor,
        TEnumAsByte<ESceneDepthPriorityGroup> InDepthPriorityGroup,
        float InThickness,
        float InDepthBias,
        bool InIsScreenSpace)
    -> void
{
    if (ck::Is_NOT_Valid(InDrawHandle))
    { return; }

    DrawWireSphere(
        InDrawHandle.Get_PrimitiveDrawInterface(),
        InOrigin,
        InColor,
        InRadius,
        InNumSides,
        InDepthPriorityGroup,
        InThickness,
        InDepthBias,
        InIsScreenSpace);
}

auto
    UCk_Utils_Scene_UE::
    DrawCapsule(
        const FCk_Handle_PrimitiveDrawInterface& InDrawHandle,
        FVector InOrigin,
        FRotator InRotation,
        float InRadius,
        float InHalfHeight,
        int32 InNumSides,
        FLinearColor InColor,
        TEnumAsByte<ESceneDepthPriorityGroup> InDepthPriorityGroup,
        float InThickness,
        float InDepthBias,
        bool InIsScreenSpace)
    -> void
{
    if (ck::Is_NOT_Valid(InDrawHandle))
    { return; }

    const auto RotMatrix = FRotationMatrix{InRotation};

    DrawWireCapsule(
        InDrawHandle.Get_PrimitiveDrawInterface(),
        InOrigin,
        RotMatrix.GetScaledAxis(EAxis::X),
        RotMatrix.GetScaledAxis(EAxis::Y),
        RotMatrix.GetScaledAxis(EAxis::Z),
        InColor,
        InRadius,
        InHalfHeight,
        InNumSides,
        InDepthPriorityGroup,
        InThickness,
        InDepthBias,
        InIsScreenSpace);
}

auto
    UCk_Utils_Scene_UE::
    DrawCylinder(
        const FCk_Handle_PrimitiveDrawInterface& InDrawHandle,
        FVector InOrigin,
        FRotator InRotation,
        float InRadius,
        float InHalfHeight,
        int32 InNumSides,
        FLinearColor InColor,
        TEnumAsByte<ESceneDepthPriorityGroup> InDepthPriorityGroup,
        float InThickness,
        float InDepthBias,
        bool InIsScreenSpace)
    -> void
{
    if (ck::Is_NOT_Valid(InDrawHandle))
    { return; }

    const auto RotMatrix = FRotationMatrix{InRotation};

    DrawWireCylinder(
        InDrawHandle.Get_PrimitiveDrawInterface(),
        InOrigin,
        RotMatrix.GetScaledAxis(EAxis::X),
        RotMatrix.GetScaledAxis(EAxis::Y),
        RotMatrix.GetScaledAxis(EAxis::Z),
        InColor,
        InRadius,
        InHalfHeight,
        InNumSides,
        InDepthPriorityGroup,
        InThickness,
        InDepthBias,
        InIsScreenSpace);
}

// ----------------------------------------------------------------------------------------------------------------
