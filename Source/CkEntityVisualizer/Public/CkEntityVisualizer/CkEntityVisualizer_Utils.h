#pragma once

#include "CkEntityVisualizer/CkEntityVisualizer_Data.h"

#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"
#include "CkIsmRenderer/Proxy/CkIsmProxy_Fragment_Data.h"
#include "CkPmg/CkPmg_Fragment_Data_DebugShapes.h"
#include "CkShapes/Box/CkShapeBox_Fragment.h"
#include "CkShapes/Capsule/CkShapeCapsule_Fragment.h"
#include "CkShapes/Cylinder/CkShapeCylinder_Fragment.h"
#include "CkShapes/Sphere/CkShapeSphere_Fragment.h"
#include "CkSpatialQuery/Probe/CkProbe_Fragment.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkEntityVisualizer_Utils.generated.h"

UCLASS(NotBlueprintable)
class CKENTITYVISUALIZER_API UCk_Utils_EntityVisualizer_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_EntityVisualizer_UE);

public:
    // One shared ISM batch per world/mesh/color/mobility. The returned child follows InAttachTo
    // through SceneNode and therefore only updates when the source transform is dirty.
    static auto
    Create_IsmPrimitive(
        FCk_Handle_Transform& InAttachTo,
        const FCk_EntityVisualizer_IsmPrimitiveParams& InParams) -> FCk_Handle_IsmProxy;

    static auto
    Create_TransformGizmo_Ism(
        FCk_Handle_Transform& InAttachTo,
        const FCk_EntityVisualizer_TransformGizmoParams& InParams = {}) -> TArray<FCk_Handle_IsmProxy>;

    static auto
    Create_TransformGizmo_Pmg(
        FCk_Handle_Transform& InAttachTo,
        const FCk_EntityVisualizer_TransformGizmoParams& InParams = {}) -> TArray<FCk_Handle_Pmg_DebugShape>;

    static auto
    Create_ProbePreview_Pmg(
        FCk_Handle_Transform& InAttachTo,
        const ck::FFragment_ShapeBox_Current& InShape,
        const ck::FFragment_Probe_DebugInfo& InDebugInfo) -> FCk_Handle_Pmg_DebugShape;

    static auto
    Create_ProbePreview_Pmg(
        FCk_Handle_Transform& InAttachTo,
        const ck::FFragment_ShapeSphere_Current& InShape,
        const ck::FFragment_Probe_DebugInfo& InDebugInfo) -> FCk_Handle_Pmg_DebugShape;

    static auto
    Create_ProbePreview_Pmg(
        FCk_Handle_Transform& InAttachTo,
        const ck::FFragment_ShapeCapsule_Current& InShape,
        const ck::FFragment_Probe_DebugInfo& InDebugInfo) -> FCk_Handle_Pmg_DebugShape;

    static auto
    Create_ProbePreview_Pmg(
        FCk_Handle_Transform& InAttachTo,
        const ck::FFragment_ShapeCylinder_Current& InShape,
        const ck::FFragment_Probe_DebugInfo& InDebugInfo) -> FCk_Handle_Pmg_DebugShape;
};
