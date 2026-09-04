#include "CkEntityVisualizer_Utils.h"

#include "CkEntityVisualizer/CkEntityVisualizer_Fragment.h"
#include "CkEntityVisualizer/CkEntityVisualizer_Log.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EditorSelectionOwner/CkEditorSelectionOwner_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkEcsExt/SceneNode/CkSceneNode_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkIsmRenderer/Proxy/CkIsmProxy_Utils.h"
#include "CkIsmRenderer/Renderer/CkIsmRenderer_TransientFactory.h"

#include "CkPmg/CkPmg_Utils_BasicShapes.h"
#include "CkPmg/CkPmg_Utils_DirectionalShapes.h"

#include <Engine/StaticMesh.h>
#include <Engine/World.h>
#include <Materials/MaterialInterface.h>

namespace ck_entity_visualizer_utils
{
    constexpr auto PersistentDuration = -1.0f;
    constexpr auto BasicShapeSize = 100.0f;

    auto AreGizmoParamsValid(const FCk_EntityVisualizer_TransformGizmoParams& InParams) -> bool
    {
        return FMath::IsFinite(InParams.Get_AxisLength()) && InParams.Get_AxisLength() > 0.0f &&
            FMath::IsFinite(InParams.Get_ShaftRadius()) && InParams.Get_ShaftRadius() > 0.0f &&
            FMath::IsFinite(InParams.Get_ArrowHeadLength()) && InParams.Get_ArrowHeadLength() > 0.0f &&
            InParams.Get_ArrowHeadLength() <= InParams.Get_AxisLength() &&
            FMath::IsFinite(InParams.Get_ArrowHeadRadius()) && InParams.Get_ArrowHeadRadius() > 0.0f;
    }

    auto
        Get_IsmVisualizerMaterial()
        -> UMaterialInterface*
    {
        return LoadObject<UMaterialInterface>(
            nullptr,
            TEXT("/CkFoundation/CkUsf/GeneratedLooks/"
                 "M_CkUsf_Look_PerInstanceHue.M_CkUsf_Look_PerInstanceHue"));
    }

    auto
        Get_Hue01(
            const FLinearColor& InColor) -> float
    {
        return InColor.LinearRGBToHSV().R / 360.0f;
    }

    auto
        Get_PrimitiveMesh(
            ECk_EntityVisualizer_Primitive InPrimitive) -> UStaticMesh*
    {
        const TCHAR* Path = nullptr;
        switch (InPrimitive)
        {
            case ECk_EntityVisualizer_Primitive::Box:
                Path = TEXT("/Engine/BasicShapes/Cube.Cube");
                break;
            case ECk_EntityVisualizer_Primitive::Sphere:
                Path = TEXT("/Engine/BasicShapes/Sphere.Sphere");
                break;
            case ECk_EntityVisualizer_Primitive::Cylinder:
                Path = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
                break;
            case ECk_EntityVisualizer_Primitive::Cone:
                Path = TEXT("/Engine/BasicShapes/Cone.Cone");
                break;
            default:
                CK_INVALID_ENUM(InPrimitive);
                return nullptr;
        }

        return LoadObject<UStaticMesh>(nullptr, Path);
    }

    auto
        Make_AxisTransform(
            const FVector& InAxis,
            float InLength,
            float InRadius,
            float InCenterDistance) -> FTransform
    {
        const auto Rotation = FQuat::FindBetweenNormals(FVector::UpVector, InAxis);
        const auto DiameterScale = (InRadius * 2.0f) / BasicShapeSize;
        const auto LengthScale = InLength / BasicShapeSize;
        return FTransform{Rotation, InAxis * InCenterDistance, FVector{DiameterScale, DiameterScale, LengthScale}};
    }

    auto
        AttachPmg(
            FCk_Handle_Transform& InAttachTo,
            FCk_Handle_Pmg_DebugShape InShape,
            const FTransform& InLocalTransform) -> FCk_Handle_Pmg_DebugShape
    {
        if (ck::Is_NOT_Valid(InShape))
        { return {}; }

        InShape.AddOrGet<ck::FTag_EntityVisualizer_Visual>();
        auto ShapeTransform = UCk_Utils_Transform_UE::CastChecked(InShape);
        const auto SceneNode = UCk_Utils_SceneNode_UE::Add(ShapeTransform, InAttachTo, InLocalTransform);
        if (ck::Is_NOT_Valid(SceneNode))
        {
            auto ShapeHandle = InShape.ConvertToHandle();
            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(ShapeHandle);
            return {};
        }

        return InShape;
    }
}

auto
    UCk_Utils_EntityVisualizer_UE::
    Create_IsmPrimitive(
        FCk_Handle_Transform& InAttachTo,
        const FCk_EntityVisualizer_IsmPrimitiveParams& InParams)
    -> FCk_Handle_IsmProxy
{
    const auto AttachTargetIsValid = ck::IsValid(InAttachTo);
    CK_ENSURE_IF_NOT(AttachTargetIsValid,
        TEXT("Unable to create an ISM visualizer for INVALID Transform [{}]"), InAttachTo)
    { return {}; }

    auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InAttachTo);
    auto* Mesh = ck_entity_visualizer_utils::Get_PrimitiveMesh(InParams.Get_Primitive());
    const auto BaseResourcesAreValid = ck::IsValid(World) && ck::IsValid(Mesh);
    CK_ENSURE_IF_NOT(BaseResourcesAreValid,
        TEXT("Unable to resolve ISM visualizer World or Mesh for Transform [{}]"), InAttachTo)
    { return {}; }

    auto* Material = ck_entity_visualizer_utils::Get_IsmVisualizerMaterial();
    const auto MaterialIsValid = ck::IsValid(Material);
    CK_ENSURE_IF_NOT(MaterialIsValid,
        TEXT("Unable to create ISM visualizer material for Transform [{}]"), InAttachTo)
    { return {}; }

    const auto Overrides = TArray<FCk_MeshMaterialOverride>{FCk_MeshMaterialOverride{0, Material}};
    auto* RendererData = UCk_Utils_IsmRenderer_TransientFactory_UE::GetOrCreate_ForMeshWithMaterialsAndCustomData(
        World, Mesh, Overrides, 1, ECk_Mobility::Movable);
    const auto RendererDataIsValid = ck::IsValid(RendererData);
    CK_ENSURE_IF_NOT(RendererDataIsValid,
        TEXT("Unable to create transient ISM renderer data for Transform [{}]"), InAttachTo)
    { return {}; }

    auto ProxyParams = FCk_Fragment_IsmProxy_ParamsData{RendererData};
    ProxyParams.Get_CustomInstanceDataDefaults().Add(FCk_CustomPrimitiveData{
        0,
        FCk_CustomPrimitiveData_Value{ck_entity_visualizer_utils::Get_Hue01(InParams.Get_Color())}});
    auto Proxy = UCk_Utils_IsmProxy_UE::Create(InAttachTo, FTransform::Identity, ProxyParams);
    if (ck::Is_NOT_Valid(Proxy))
    { return {}; }

    Proxy.AddOrGet<ck::FTag_EntityVisualizer_Visual>();

#if WITH_EDITOR
    if (Proxy.Has<ck::FFragment_EditorSelectionOwner>())
    { Proxy.Remove<ck::FFragment_EditorSelectionOwner>(); }
#endif

    auto ProxyTransform = UCk_Utils_Transform_UE::CastChecked(Proxy);
    const auto SceneNode = UCk_Utils_SceneNode_UE::Add(
        ProxyTransform, InAttachTo, InParams.Get_LocalTransform());
    if (ck::Is_NOT_Valid(SceneNode))
    {
        auto ProxyHandle = Proxy.ConvertToHandle();
        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(ProxyHandle);
        return {};
    }

    return Proxy;
}

auto
    UCk_Utils_EntityVisualizer_UE::
    Create_TransformGizmo_Ism(
        FCk_Handle_Transform& InAttachTo,
        const FCk_EntityVisualizer_TransformGizmoParams& InParams)
    -> TArray<FCk_Handle_IsmProxy>
{
    const auto ParamsAreValid = ck_entity_visualizer_utils::AreGizmoParamsValid(InParams);
    CK_ENSURE_IF_NOT(ParamsAreValid,
        TEXT("Unable to create an ISM transform gizmo with invalid dimensions"))
    { return {}; }

    auto Result = TArray<FCk_Handle_IsmProxy>{};
    Result.Reserve(6);

    const auto ShaftLength = FMath::Max(0.0f, InParams.Get_AxisLength() - InParams.Get_ArrowHeadLength());
    const auto Axes = TArray<TPair<FVector, FLinearColor>>{
        {FVector::ForwardVector, FLinearColor::Red},
        {FVector::RightVector, FLinearColor::Green},
        {FVector::UpVector, FLinearColor::Blue},
    };

    for (const auto& [Axis, Color] : Axes)
    {
        auto ShaftParams = FCk_EntityVisualizer_IsmPrimitiveParams{};
        ShaftParams.Set_Primitive(ECk_EntityVisualizer_Primitive::Cylinder);
        ShaftParams.Set_Color(Color);
        ShaftParams.Set_LocalTransform(ck_entity_visualizer_utils::Make_AxisTransform(
            Axis, ShaftLength, InParams.Get_ShaftRadius(), ShaftLength * 0.5f));

        auto Shaft = Create_IsmPrimitive(InAttachTo, ShaftParams);
        if (ck::Is_NOT_Valid(Shaft))
        {
            for (auto& Created : Result)
            {
                auto Handle = Created.ConvertToHandle();
                UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(Handle);
            }
            return {};
        }
        Result.Add(Shaft);

        auto HeadParams = FCk_EntityVisualizer_IsmPrimitiveParams{};
        HeadParams.Set_Primitive(ECk_EntityVisualizer_Primitive::Cone);
        HeadParams.Set_Color(Color);
        HeadParams.Set_LocalTransform(ck_entity_visualizer_utils::Make_AxisTransform(
            Axis,
            InParams.Get_ArrowHeadLength(),
            InParams.Get_ArrowHeadRadius(),
            ShaftLength + InParams.Get_ArrowHeadLength() * 0.5f));

        auto Head = Create_IsmPrimitive(InAttachTo, HeadParams);
        if (ck::Is_NOT_Valid(Head))
        {
            for (auto& Created : Result)
            {
                auto Handle = Created.ConvertToHandle();
                UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(Handle);
            }
            return {};
        }
        Result.Add(Head);
    }

    return Result;
}

auto
    UCk_Utils_EntityVisualizer_UE::
    Create_TransformGizmo_Pmg(
        FCk_Handle_Transform& InAttachTo,
        const FCk_EntityVisualizer_TransformGizmoParams& InParams)
    -> TArray<FCk_Handle_Pmg_DebugShape>
{
    const auto ParamsAreValid = ck_entity_visualizer_utils::AreGizmoParamsValid(InParams);
    CK_ENSURE_IF_NOT(ParamsAreValid,
        TEXT("Unable to create a PMG transform gizmo with invalid dimensions"))
    { return {}; }

    auto Result = TArray<FCk_Handle_Pmg_DebugShape>{};
    Result.Reserve(3);

    const auto Axes = TArray<TPair<FVector, FLinearColor>>{
        {FVector::ForwardVector, FLinearColor::Red},
        {FVector::RightVector, FLinearColor::Green},
        {FVector::UpVector, FLinearColor::Blue},
    };

    for (const auto& [Axis, Color] : Axes)
    {
        const auto LocalTransform = FTransform{
            FQuat::FindBetweenNormals(FVector::ForwardVector, Axis), FVector::ZeroVector};
        auto Shape = UCk_Utils_Pmg_DirectionalShapes::Create_Arrow(
            InAttachTo,
            FTransform::Identity,
            InParams.Get_AxisLength(),
            InParams.Get_ShaftRadius() * 2.0f,
            InParams.Get_ArrowHeadLength() / InParams.Get_AxisLength(),
            InParams.Get_ArrowHeadRadius() / InParams.Get_ShaftRadius(),
            Color,
            true,
            1.0f,
            ECk_Plane_Axis::XY,
            ck_entity_visualizer_utils::PersistentDuration);

        Shape = ck_entity_visualizer_utils::AttachPmg(InAttachTo, Shape, LocalTransform);
        if (ck::Is_NOT_Valid(Shape))
        {
            for (auto& Created : Result)
            {
                auto Handle = Created.ConvertToHandle();
                UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(Handle);
            }
            return {};
        }
        Result.Add(Shape);
    }

    return Result;
}

auto
    UCk_Utils_EntityVisualizer_UE::
    Create_ProbePreview_Pmg(
        FCk_Handle_Transform& InAttachTo,
        const ck::FFragment_ShapeBox_Current& InShape,
        const ck::FFragment_Probe_DebugInfo& InDebugInfo)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Shape = UCk_Utils_Pmg_BasicShapes::Create_Box(
        InAttachTo, FTransform::Identity, InShape.Get_Dimensions().Get_HalfExtents(), ECk_Plane_Axis::XY,
        InDebugInfo.Get_Color(), true, InDebugInfo.Get_LineThickness(), ck_entity_visualizer_utils::PersistentDuration);
    return ck_entity_visualizer_utils::AttachPmg(InAttachTo, Shape, FTransform::Identity);
}

auto
    UCk_Utils_EntityVisualizer_UE::
    Create_ProbePreview_Pmg(
        FCk_Handle_Transform& InAttachTo,
        const ck::FFragment_ShapeSphere_Current& InShape,
        const ck::FFragment_Probe_DebugInfo& InDebugInfo)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Shape = UCk_Utils_Pmg_BasicShapes::Create_Sphere(
        InAttachTo, FTransform::Identity, InShape.Get_Dimensions().Get_Radius(), 12, 8, ECk_Plane_Axis::XY,
        InDebugInfo.Get_Color(), true, InDebugInfo.Get_LineThickness(), ck_entity_visualizer_utils::PersistentDuration);
    return ck_entity_visualizer_utils::AttachPmg(InAttachTo, Shape, FTransform::Identity);
}

auto
    UCk_Utils_EntityVisualizer_UE::
    Create_ProbePreview_Pmg(
        FCk_Handle_Transform& InAttachTo,
        const ck::FFragment_ShapeCapsule_Current& InShape,
        const ck::FFragment_Probe_DebugInfo& InDebugInfo)
    -> FCk_Handle_Pmg_DebugShape
{
    const auto& Dimensions = InShape.Get_Dimensions();
    auto Shape = UCk_Utils_Pmg_BasicShapes::Create_Capsule(
        InAttachTo, FTransform::Identity, Dimensions.Get_Radius(), Dimensions.Get_HalfHeight(), 12, 6,
        ECk_Plane_Axis::XY, InDebugInfo.Get_Color(), true, InDebugInfo.Get_LineThickness(),
        ck_entity_visualizer_utils::PersistentDuration);
    return ck_entity_visualizer_utils::AttachPmg(InAttachTo, Shape, FTransform::Identity);
}

auto
    UCk_Utils_EntityVisualizer_UE::
    Create_ProbePreview_Pmg(
        FCk_Handle_Transform& InAttachTo,
        const ck::FFragment_ShapeCylinder_Current& InShape,
        const ck::FFragment_Probe_DebugInfo& InDebugInfo)
    -> FCk_Handle_Pmg_DebugShape
{
    const auto& Dimensions = InShape.Get_Dimensions();
    auto Shape = UCk_Utils_Pmg_BasicShapes::Create_Cylinder(
        InAttachTo, FTransform::Identity, Dimensions.Get_Radius(), Dimensions.Get_HalfHeight() * 2.0f, 12,
        ECk_Plane_Axis::XY, InDebugInfo.Get_Color(), true, InDebugInfo.Get_LineThickness(),
        ck_entity_visualizer_utils::PersistentDuration);
    return ck_entity_visualizer_utils::AttachPmg(InAttachTo, Shape, FTransform::Identity);
}
