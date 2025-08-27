#include "CkWorldSpaceWidget_Utils.h"

#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkEcsExt/SceneNode/CkSceneNode_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

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
    auto Widget = InParams.Get_Widget().Get();

    CK_ENSURE_IF_NOT(ck::IsValid(Widget), TEXT("Cannot Create WorldSpaceWidget because the Widget supplied is INVALID"))
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
    auto Widget = InParams.Get_Widget().Get();

    CK_ENSURE_IF_NOT(ck::IsValid(Widget), TEXT("Cannot Create WorldSpaceWidget because the Widget supplied is INVALID"))
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
    CK_ENSURE_IF_NOT(ck::IsValid(InParams.Get_Widget()), TEXT("Cannot Create WorldSpaceWidget because the Widget supplied is INVALID"))
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
    auto ContentWidget = InParams.Get_Widget().Get();
    auto WrapperWidget = UCk_WorldSpaceWidget_Wrapper_UE::Request_WrapWidget(ContentWidget);

    InHandle.Add<ck::FFragment_WorldSpaceWidget_Params>(InParams);
    InHandle.Add<ck::FFragment_WorldSpaceWidget_Current>(WrapperWidget);

    if (InParams.Get_ScalingInfo().Get_ScalingPolicy() == ECk_WorldSpaceWidget_Scaling_Policy::ScaleWithDistance)
    {
        InHandle.Add<ck::FTag_WorldSpaceWidget_NeedsUpdateScaling>();
    }

    auto ViewportOperation = InParams.Get_InitialViewportOperation();
    switch (ViewportOperation)
    {
        case ECk_UI_Widget_ViewportOperation::DoNothing:
        {
            break;
        }
        case ECk_UI_Widget_ViewportOperation::AddToViewport:
        {
            ContentWidget->AddToViewport();
            break;
        }
        case ECk_UI_Widget_ViewportOperation::RemoveFromViewport:
        {
            ContentWidget->RemoveFromParent();
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
    return InWorldSpaceWidgetHandle.Get<ck::FFragment_WorldSpaceWidget_Current>()._ContentWidgetHardRef.Get();
}

// --------------------------------------------------------------------------------------------------------------------
