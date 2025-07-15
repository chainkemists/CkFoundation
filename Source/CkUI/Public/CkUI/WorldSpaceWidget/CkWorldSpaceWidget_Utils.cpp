#include "CkWorldSpaceWidget_Utils.h"

#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkEcs/SceneNode/CkSceneNode_Utils.h"
#include "CkEcs/Transform/CkTransform_Utils.h"

#include "CkUI/WorldSpaceWidget/CkWorldSpaceWidget_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_WorldSpaceWidget_UE, FCk_Handle_WorldSpaceWidget,
    ck::FFragment_WorldSpaceWidget_Current, ck::FFragment_WorldSpaceWidget_Params);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_WorldSpaceWidget_UE::
    Create_AtLocation(
        FVector InLocation,
        const FCk_Fragment_WorldSpaceWidget_ParamsData& InParams)
    -> FCk_Handle_WorldSpaceWidget
{
    const auto& Widget = InParams.Get_Widget().Get();

    CK_ENSURE_IF_NOT(ck::IsValid(Widget),
        TEXT("Cannot Create WorldSpaceWidget because the Widget supplied is INVALID!"))
    { return {}; }

    auto EntityAtLocation = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(Widget);
    UCk_Utils_Handle_UE::Set_DebugName(EntityAtLocation, *ck::Format_UE(TEXT("WORLD SPACE WIDGET (At [{}])"), InLocation));

    auto EntityAtLocation_AsTransform = UCk_Utils_Transform_UE::Add(EntityAtLocation, FTransform{InLocation}, ECk_Replication::DoesNotReplicate);

    return DoAdd(EntityAtLocation_AsTransform, InParams);
}

auto
    UCk_Utils_WorldSpaceWidget_UE::
    CreateAndAttach_ToUnrealComponent(
        USceneComponent* InAttachTo,
        const FCk_Fragment_WorldSpaceWidget_ParamsData& InParams)
    -> FCk_Handle_WorldSpaceWidget
{
    const auto& Widget = InParams.Get_Widget().Get();

    CK_ENSURE_IF_NOT(ck::IsValid(Widget),
        TEXT("Cannot Create WorldSpaceWidget because the Widget supplied is INVALID!"))
    { return {}; }

    auto EntityAtLocation = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(Widget);
    UCk_Utils_Handle_UE::Set_DebugName(EntityAtLocation, *ck::Format_UE(TEXT("WORLD SPACE WIDGET (Attached to [{}])"), InAttachTo));

    auto EntityAtLocation_AsTransform = UCk_Utils_Transform_UE::AddAndAttachToUnrealComponent(EntityAtLocation, InAttachTo, ECk_Replication::DoesNotReplicate);

    return DoAdd(EntityAtLocation_AsTransform, InParams);
}

auto
    UCk_Utils_WorldSpaceWidget_UE::
    CreateAndAttach_ToEntity(
        FCk_Handle_Transform& InAttachTo,
        const FCk_Fragment_WorldSpaceWidget_ParamsData& InParams)
    -> FCk_Handle_WorldSpaceWidget
{
    CK_ENSURE_IF_NOT(ck::IsValid(InParams.Get_Widget()),
        TEXT("Cannot Create WorldSpaceWidget because the Widget supplied is INVALID!"))
    { return {}; }

    auto WorldSpaceWidgetSceneNode = UCk_Utils_SceneNode_UE::Create(InAttachTo, FTransform::Identity);
    UCk_Utils_Handle_UE::Set_DebugName(WorldSpaceWidgetSceneNode, *ck::Format_UE(TEXT("WORLD SPACE WIDGET (Attached to [{}])"), InAttachTo));

    auto WorldSpaceWidgetSceneNode_AsTransform = UCk_Utils_Transform_UE::Cast(WorldSpaceWidgetSceneNode);

    return DoAdd(WorldSpaceWidgetSceneNode_AsTransform, InParams);
}

auto
    UCk_Utils_WorldSpaceWidget_UE::
    DoAdd(
        FCk_Handle_Transform& InHandle,
        const FCk_Fragment_WorldSpaceWidget_ParamsData& InParams)
    -> FCk_Handle_WorldSpaceWidget
{
    const auto& Widget = InParams.Get_Widget().Get();
    InHandle.Add<ck::FFragment_WorldSpaceWidget_Params>(InParams);
    InHandle.Add<ck::FFragment_WorldSpaceWidget_Current>(Widget);

    switch (const auto& ViewportOperation = InParams.Get_InitialViewportOperation())
    {
        case ECk_UI_Widget_ViewportOperation::DoNothing:
        {
            break;
        }
        case ECk_UI_Widget_ViewportOperation::AddToViewport:
        {
            Widget->AddToViewport();
            break;
        }
        case ECk_UI_Widget_ViewportOperation::RemoveFromViewport:
        {
            Widget->RemoveFromParent();
            break;
        }
        default:
        {
            CK_INVALID_ENUM(ViewportOperation);
            break;
        }
    }

    return Cast(InHandle);
}

auto
    UCk_Utils_WorldSpaceWidget_UE::
    Get_Instance(
        const FCk_Handle_WorldSpaceWidget& InWorldSpaceWidgetHandle)
    -> UUserWidget*
{
    return InWorldSpaceWidgetHandle.Get<ck::FFragment_WorldSpaceWidget_Current>()._WidgetHardRef.Get();
}


// --------------------------------------------------------------------------------------------------------------------
