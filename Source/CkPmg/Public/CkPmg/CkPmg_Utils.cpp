#include "CkPmg_Utils.h"

#include "CkPmg/CkPmg_Log.h"
#include "CkPmg_Fragment.h"

#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#if WITH_EDITOR
#include "AssetUtils/CreateStaticMeshUtil.h"
#include "Engine/StaticMesh.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "ProceduralMeshConversion.h"
#endif

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_Donut_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_Pmg_Donut_ParamsData& InParams)
    -> FCk_Handle_Pmg_Donut
{
    ck::pmg::VeryVerbose(TEXT("Adding Pmg Donut feature to Entity [{}]"), InHandle);

    CK_ENSURE_IF_NOT(InParams.Get_InnerRadius() < InParams.Get_OuterRadius(),
        TEXT("Inner radius must be less than outer radius for Entity [{}]"), InHandle)
    { return {}; }

    CK_ENSURE_IF_NOT(InParams.Get_Segments() >= 3,
        TEXT("Segments must be at least 3 for Entity [{}]"), InHandle)
    { return {}; }

    auto& ParamsFragment = InHandle.Add<ck::FFragment_Pmg_Donut_Params>();
    ParamsFragment._Params = InParams;

    InHandle.Add<ck::FFragment_Pmg_Donut_Current>();
    InHandle.Add<ck::FTag_Pmg_Donut_NeedsSetup>();

    UCk_Utils_Handle_UE::Set_DebugName(InHandle, TEXT("Pmg: Donut"));

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Pmg_Donut_UE, FCk_Handle_Pmg_Donut,
    ck::FFragment_Pmg_Donut_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_Donut_UE::
    Request_UpdateParams(
        FCk_Handle_Pmg_Donut& InDonut,
        const FCk_Request_Pmg_Donut_UpdateParams& InRequest)
    -> FCk_Handle_Pmg_Donut
{
    ck::pmg::Verbose(TEXT("Requesting param update for Pmg Donut [{}]"), InDonut);

    InDonut.AddOrGet<ck::FFragment_Pmg_Donut_UpdateParams>() = InRequest;

    return InDonut;
}

auto
    UCk_Utils_Pmg_Donut_UE::
    Request_SetInnerRadius(
        FCk_Handle_Pmg_Donut& InDonut,
        float InValue)
    -> FCk_Handle_Pmg_Donut
{
    InDonut.AddOrGet<ck::FFragment_Pmg_Donut_UpdateParams>().Set_InnerRadius(InValue);
    return InDonut;
}

auto
    UCk_Utils_Pmg_Donut_UE::
    Request_SetOuterRadius(
        FCk_Handle_Pmg_Donut& InDonut,
        float InValue)
    -> FCk_Handle_Pmg_Donut
{
    InDonut.AddOrGet<ck::FFragment_Pmg_Donut_UpdateParams>().Set_OuterRadius(InValue);
    return InDonut;
}

auto
    UCk_Utils_Pmg_Donut_UE::
    Request_SetSegments(
        FCk_Handle_Pmg_Donut& InDonut,
        int32 InValue)
    -> FCk_Handle_Pmg_Donut
{
    InDonut.AddOrGet<ck::FFragment_Pmg_Donut_UpdateParams>().Set_Segments(InValue);
    return InDonut;
}

auto
    UCk_Utils_Pmg_Donut_UE::
    Request_SetFillAngle(
        FCk_Handle_Pmg_Donut& InDonut,
        float InValue)
    -> FCk_Handle_Pmg_Donut
{
    InDonut.AddOrGet<ck::FFragment_Pmg_Donut_UpdateParams>().Set_FillAngle(InValue);
    return InDonut;
}

auto
    UCk_Utils_Pmg_Donut_UE::
    Request_SetMaterial(
        FCk_Handle_Pmg_Donut& InDonut,
        UMaterialInterface* InValue)
    -> FCk_Handle_Pmg_Donut
{
    InDonut.AddOrGet<ck::FFragment_Pmg_Donut_UpdateParams>().Set_Material(InValue);
    return InDonut;
}

auto
    UCk_Utils_Pmg_Donut_UE::
    Request_SetEnableCollision(
        FCk_Handle_Pmg_Donut& InDonut,
        bool InValue)
    -> FCk_Handle_Pmg_Donut
{
    InDonut.AddOrGet<ck::FFragment_Pmg_Donut_UpdateParams>().Set_EnableCollision(InValue);
    return InDonut;
}

auto
    UCk_Utils_Pmg_Donut_UE::
    Request_SetRenderMode(
        FCk_Handle_Pmg_Donut& InDonut,
        ECk_Pmg_RenderMode InValue)
    -> FCk_Handle_Pmg_Donut
{
    InDonut.AddOrGet<ck::FFragment_Pmg_Donut_UpdateParams>().Set_RenderMode(InValue);
    return InDonut;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_Donut_UE::
    Get_InnerRadius(
        const FCk_Handle_Pmg_Donut& InDonut)
    -> float
{
    CK_ENSURE_IF_NOT(ck::IsValid(InDonut), TEXT("Invalid Pmg Donut handle"))
    { return 0.0f; }

    CK_ENSURE_IF_NOT(InDonut.Has<ck::FFragment_Pmg_Donut_Current>(),
        TEXT("Pmg Donut [{}] has no Current fragment"), InDonut)
    { return 0.0f; }

    const auto& Current = InDonut.Get<ck::FFragment_Pmg_Donut_Current>();
    return Current.Get_InnerRadius();
}

auto
    UCk_Utils_Pmg_Donut_UE::
    Get_OuterRadius(
        const FCk_Handle_Pmg_Donut& InDonut)
    -> float
{
    CK_ENSURE_IF_NOT(ck::IsValid(InDonut), TEXT("Invalid Pmg Donut handle"))
    { return 0.0f; }

    CK_ENSURE_IF_NOT(InDonut.Has<ck::FFragment_Pmg_Donut_Current>(),
        TEXT("Pmg Donut [{}] has no Current fragment"), InDonut)
    { return 0.0f; }

    const auto& Current = InDonut.Get<ck::FFragment_Pmg_Donut_Current>();
    return Current.Get_OuterRadius();
}

auto
    UCk_Utils_Pmg_Donut_UE::
    Get_Segments(
        const FCk_Handle_Pmg_Donut& InDonut)
    -> int32
{
    CK_ENSURE_IF_NOT(ck::IsValid(InDonut), TEXT("Invalid Pmg Donut handle"))
    { return 0; }

    CK_ENSURE_IF_NOT(InDonut.Has<ck::FFragment_Pmg_Donut_Current>(),
        TEXT("Pmg Donut [{}] has no Current fragment"), InDonut)
    { return 0; }

    const auto& Current = InDonut.Get<ck::FFragment_Pmg_Donut_Current>();
    return Current.Get_Segments();
}

auto
    UCk_Utils_Pmg_Donut_UE::
    Get_FillAngle(
        const FCk_Handle_Pmg_Donut& InDonut)
    -> float
{
    CK_ENSURE_IF_NOT(ck::IsValid(InDonut), TEXT("Invalid Pmg Donut handle"))
    { return 0.0f; }

    CK_ENSURE_IF_NOT(InDonut.Has<ck::FFragment_Pmg_Donut_Current>(),
        TEXT("Pmg Donut [{}] has no Current fragment"), InDonut)
    { return 0.0f; }

    const auto& Current = InDonut.Get<ck::FFragment_Pmg_Donut_Current>();
    return Current.Get_FillAngle();
}

auto
    UCk_Utils_Pmg_Donut_UE::
    Get_RenderMode(
        const FCk_Handle_Pmg_Donut& InDonut)
    -> ECk_Pmg_RenderMode
{
    CK_ENSURE_IF_NOT(ck::IsValid(InDonut), TEXT("Invalid Pmg Donut handle"))
    { return ECk_Pmg_RenderMode::DoubleSided; }

    CK_ENSURE_IF_NOT(InDonut.Has<ck::FFragment_Pmg_Donut_Current>(),
        TEXT("Pmg Donut [{}] has no Current fragment"), InDonut)
    { return ECk_Pmg_RenderMode::DoubleSided; }

    const auto& Current = InDonut.Get<ck::FFragment_Pmg_Donut_Current>();
    return Current.Get_RenderMode();
}

// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_Pmg_DebugShape_ParamsData& InParams,
        FTransform InTransform)
    -> FCk_Handle_Pmg_DebugShape
{
    ck::pmg::VeryVerbose(TEXT("Adding Pmg DebugShape feature to Entity [{}]"), InHandle);

    auto& ParamsFragment = InHandle.Add<ck::FFragment_Pmg_DebugShape_Params>();
    ParamsFragment._Params = InParams;

    InHandle.Add<ck::FFragment_Pmg_DebugShape_Current>();
    InHandle.Add<ck::FTag_Pmg_DebugShape_NeedsSetup>();

    UCk_Utils_Transform_UE::Add(InHandle, InTransform, ECk_Replication::DoesNotReplicate);
    UCk_Utils_Handle_UE::Set_DebugName(InHandle, TEXT("Pmg: DebugShape"));

    return Cast(InHandle);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Create(
        FCk_Handle& InOwningEntity,
        const FCk_Fragment_Pmg_DebugShape_ParamsData& InParams,
        FTransform InTransform)
    -> FCk_Handle_Pmg_DebugShape
{
    ck::pmg::VeryVerbose(TEXT("Creating Pmg DebugShape entity from Owner [{}]"), InOwningEntity);

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwningEntity);
    CK_ENSURE_IF_NOT(ck::IsValid(NewEntity), TEXT("Failed to create entity from Owner [{}]"), InOwningEntity)
    { return {}; }

    return Add(NewEntity, InParams, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Create_TransientOwner(
        const UObject* InWorldContextObject,
        const FCk_Fragment_Pmg_DebugShape_ParamsData& InParams,
        FTransform InTransform)
    -> FCk_Handle_Pmg_DebugShape
{
    CK_ENSURE_IF_NOT(ck::IsValid(InWorldContextObject), TEXT("Invalid WorldContextObject"))
    { return {}; }

    const auto World = InWorldContextObject->GetWorld();
    CK_ENSURE_IF_NOT(ck::IsValid(World), TEXT("Invalid World"))
    { return {}; }

    ck::pmg::VeryVerbose(TEXT("Creating Pmg DebugShape entity with transient owner"));

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(World);
    CK_ENSURE_IF_NOT(ck::IsValid(NewEntity), TEXT("Failed to create entity with transient owner"))
    { return {}; }

    return Add(NewEntity, InParams, InTransform);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Pmg_DebugShape_UE, FCk_Handle_Pmg_DebugShape,
    ck::FFragment_Pmg_DebugShape_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Add_Sphere(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InRadius,
        int32 InSegments,
        int32 InRings,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Sphere);
    Params.Set_Size(InRadius);
    Params.Set_Segments(InSegments);
    Params.Set_Rings(InRings);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_Duration(FCk_Time(InDuration));

    return Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Add_Box(
        FCk_Handle& InHandle,
        FTransform InTransform,
        FVector InExtent,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Box);
    Params.Set_Size(InExtent.X);
    Params.Set_SecondarySize(InExtent.Y);
    Params.Set_TertiarySize(InExtent.Z);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_Duration(FCk_Time(InDuration));

    return Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Add_Circle(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InRadius,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        bool InDrawDirectionLine,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Circle);
    Params.Set_Size(InRadius);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DrawDirectionLine(InDrawDirectionLine);
    Params.Set_Duration(FCk_Time(InDuration));

    return Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Add_Cone(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InRadius,
        float InHeight,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Cone);
    Params.Set_Size(InRadius);
    Params.Set_SecondarySize(InHeight);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_Duration(FCk_Time(InDuration));

    return Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Add_Cylinder(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InRadius,
        float InHeight,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Cylinder);
    Params.Set_Size(InRadius);
    Params.Set_SecondarySize(InHeight);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_Duration(FCk_Time(InDuration));

    return Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Add_Capsule(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InRadius,
        float InHalfHeight,
        int32 InSegments,
        int32 InRings,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Capsule);
    Params.Set_Size(InRadius);
    Params.Set_SecondarySize(InHalfHeight);
    Params.Set_Segments(InSegments);
    Params.Set_Rings(InRings);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_Duration(FCk_Time(InDuration));

    return Add(InHandle, Params, InTransform);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Create_Sphere(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InRadius,
        int32 InSegments,
        int32 InRings,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Sphere);
    Params.Set_Size(InRadius);
    Params.Set_Segments(InSegments);
    Params.Set_Rings(InRings);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_Duration(FCk_Time(InDuration));

    return Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Create_Box(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        FVector InExtent,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Box);
    Params.Set_Size(InExtent.X);
    Params.Set_SecondarySize(InExtent.Y);
    Params.Set_TertiarySize(InExtent.Z);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_Duration(FCk_Time(InDuration));

    return Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Create_Circle(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InRadius,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        bool InDrawDirectionLine,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Circle);
    Params.Set_Size(InRadius);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DrawDirectionLine(InDrawDirectionLine);
    Params.Set_Duration(FCk_Time(InDuration));

    return Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Create_Cone(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InRadius,
        float InHeight,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Cone);
    Params.Set_Size(InRadius);
    Params.Set_SecondarySize(InHeight);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_Duration(FCk_Time(InDuration));

    return Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Create_Cylinder(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InRadius,
        float InHeight,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Cylinder);
    Params.Set_Size(InRadius);
    Params.Set_SecondarySize(InHeight);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_Duration(FCk_Time(InDuration));

    return Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Create_Capsule(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InRadius,
        float InHalfHeight,
        int32 InSegments,
        int32 InRings,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Capsule);
    Params.Set_Size(InRadius);
    Params.Set_SecondarySize(InHalfHeight);
    Params.Set_Segments(InSegments);
    Params.Set_Rings(InRings);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_Duration(FCk_Time(InDuration));

    return Create(InOwningEntity, Params, InTransform);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_DebugShape_UE::
    DrawFilledSphere(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InRadius,
        int32 InSegments,
        int32 InRings,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Sphere);
    Params.Set_Size(InRadius);
    Params.Set_Segments(InSegments);
    Params.Set_Rings(InRings);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(FRotator::ZeroRotator, InCenter, FVector::OneVector);
    return Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    DrawFilledBox(
        const UObject* InWorldContextObject,
        FVector InCenter,
        FVector InExtent,
        FRotator InRotation,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Box);
    Params.Set_Size(InExtent.X);
    Params.Set_SecondarySize(InExtent.Y);
    Params.Set_TertiarySize(InExtent.Z);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(InRotation, InCenter, FVector::OneVector);
    return Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    DrawFilledCircle(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InRadius,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        bool InDrawDirectionLine,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Circle);
    Params.Set_Size(InRadius);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DrawDirectionLine(InDrawDirectionLine);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(FRotator::ZeroRotator, InCenter, FVector::OneVector);
    return Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    DrawFilledCone(
        const UObject* InWorldContextObject,
        FVector InOrigin,
        float InRadius,
        float InHeight,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Cone);
    Params.Set_Size(InRadius);
    Params.Set_SecondarySize(InHeight);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(FRotator::ZeroRotator, InOrigin, FVector::OneVector);
    return Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    DrawFilledCylinder(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InRadius,
        float InHeight,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Cylinder);
    Params.Set_Size(InRadius);
    Params.Set_SecondarySize(InHeight);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(FRotator::ZeroRotator, InCenter, FVector::OneVector);
    return Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    DrawFilledCapsule(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InRadius,
        float InHalfHeight,
        FRotator InRotation,
        int32 InSegments,
        int32 InRings,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Capsule);
    Params.Set_Size(InRadius);
    Params.Set_SecondarySize(InHalfHeight);
    Params.Set_Segments(InSegments);
    Params.Set_Rings(InRings);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(InRotation, InCenter, FVector::OneVector);
    return Create_TransientOwner(InWorldContextObject, Params, Transform);
}

// --------------------------------------------------------------------------------------------------------------------
