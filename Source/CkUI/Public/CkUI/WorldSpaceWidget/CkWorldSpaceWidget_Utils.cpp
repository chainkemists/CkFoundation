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
    CK_ENSURE_IF_NOT(ck::IsValid(InParams.Get_Widget()), TEXT("Cannot Create WorldSpaceWidget because the Widget supplied is INVALID"))
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InAttachTo, ck::IsValid_Policy_NullptrOnly{}), TEXT("Cannot Create WorldSpaceWidget because the Component to attach to is INVALID"))
    { return {}; }

    auto EntityAtLocation = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(InAttachTo);
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
    InHandle.Add<ck::FFragment_WorldSpaceWidget_Params>(InParams);

    if (InParams.Get_RenderMode() == ECk_WorldSpaceWidget_RenderMode::WorldComponent)
    {
        return DoAdd_WorldComponent(InHandle, InParams);
    }

    return DoAdd_ScreenOverlay(InHandle, InParams);
}

auto
    UCk_Utils_WorldSpaceWidget_UE::
    DoAdd_ScreenOverlay(
        FCk_Handle_Transform& InHandle,
        const FCk_Fragment_WorldSpaceWidget_ParamsData& InParams)
    -> FCk_Handle_WorldSpaceWidget
{
    const auto ContentWidget = InParams.Get_Widget().Get();
    const auto ZOrder = InParams.Get_ZOrder();
    auto WrapperWidget = UCk_WorldSpaceWidget_Wrapper_UE::Request_WrapWidget(ContentWidget, ZOrder);

    InHandle.Add<ck::FFragment_WorldSpaceWidget_Current>(WrapperWidget);

    if (InParams.Get_ScalingInfo().Get_ScalingPolicy() == ECk_WorldSpaceWidget_Scaling_Policy::ScaleWithDistance)
    {
        InHandle.Add<ck::FTag_WorldSpaceWidget_NeedsUpdateScaling>();
    }

    auto& Current = InHandle.Get<ck::FFragment_WorldSpaceWidget_Current>();

    switch (const auto& ViewportOperation = InParams.Get_InitialViewportOperation())
    {
        case ECk_UI_Widget_ViewportOperation::DoNothing:
        {
            Current._Enabled = false;
            break;
        }
        case ECk_UI_Widget_ViewportOperation::AddToViewport:
        {
            ContentWidget->AddToViewport();
            Current._Enabled = true;
            break;
        }
        case ECk_UI_Widget_ViewportOperation::RemoveFromViewport:
        {
            ContentWidget->RemoveFromParent();
            Current._Enabled = false;
            break;
        }
        default:
        {
            CK_INVALID_ENUM(ViewportOperation);
            break;
        }
    }

    if (ck::IsValid(WrapperWidget))
    {
        WrapperWidget->SetRenderOpacity(Current._Enabled ? 1.0f : 0.0f);
    }

    return Cast(InHandle);
}

auto
    UCk_Utils_WorldSpaceWidget_UE::
    DoAdd_WorldComponent(
        FCk_Handle_Transform& InHandle,
        const FCk_Fragment_WorldSpaceWidget_ParamsData& InParams)
    -> FCk_Handle_WorldSpaceWidget
{
    const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);

    CK_ENSURE_IF_NOT(ck::IsValid(World, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Cannot Create WorldComponent WorldSpaceWidget: invalid World for entity [{}]"), InHandle)
    { return {}; }

    const auto ContentWidget = InParams.Get_Widget().Get();
    const auto& WorldComponentInfo = InParams.Get_WorldComponentInfo();

    auto WidgetComponent = NewObject<UWidgetComponent>(World);

    CK_ENSURE_IF_NOT(ck::IsValid(WidgetComponent, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Failed to create UWidgetComponent for WorldSpaceWidget"))
    { return {}; }

    WidgetComponent->SetWidgetSpace(EWidgetSpace::World);
    WidgetComponent->SetDrawSize(FVector2D{WorldComponentInfo.Get_DrawSize()});
    WidgetComponent->SetPivot(WorldComponentInfo.Get_Pivot());
    WidgetComponent->SetBlendMode(WorldComponentInfo.Get_BlendMode());
    WidgetComponent->SetGeometryMode(WorldComponentInfo.Get_GeometryMode());
    WidgetComponent->SetTwoSided(WorldComponentInfo.Get_TwoSided());
    WidgetComponent->SetMobility(EComponentMobility::Movable);

    constexpr auto TickEvenWhenOffscreen = true;
    WidgetComponent->SetTickWhenOffscreen(TickEvenWhenOffscreen);

    if (const auto OverrideMaterial = WorldComponentInfo.Get_OverrideMaterial();
        ck::IsValid(OverrideMaterial))
    {
        WidgetComponent->SetMaterial(0, OverrideMaterial);
    }

    WidgetComponent->RegisterComponentWithWorld(World);

    // Assign the explicit instance — do NOT rely on the component instantiating
    // from a class (InitWidget is unreliable for runtime-created components and
    // leaves GetWidget() null). Must be done after registration.
    WidgetComponent->SetWidget(ContentWidget);

    InHandle.Add<ck::FFragment_WorldSpaceWidget_Current>(WidgetComponent, ContentWidget);

    auto TypedHandle = Cast(InHandle);

    const auto InitiallyEnabled = InParams.Get_InitialViewportOperation() == ECk_UI_Widget_ViewportOperation::AddToViewport;
    Request_SetEnabled(TypedHandle, InitiallyEnabled);

    return TypedHandle;
}

auto
    UCk_Utils_WorldSpaceWidget_UE::
    Request_SetEnabled(
        FCk_Handle_WorldSpaceWidget& InWorldSpaceWidgetHandle,
        bool InEnabled)
    -> FCk_Handle_WorldSpaceWidget
{
    auto& Current = InWorldSpaceWidgetHandle.Get<ck::FFragment_WorldSpaceWidget_Current>();
    Current._Enabled = InEnabled;

    if (InWorldSpaceWidgetHandle.Get<ck::FFragment_WorldSpaceWidget_Params>().Get_RenderMode() == ECk_WorldSpaceWidget_RenderMode::WorldComponent)
    {
        if (const auto WidgetComponent = Current.Get_WidgetComponent().Get();
            ck::IsValid(WidgetComponent, ck::IsValid_Policy_NullptrOnly{}))
        {
            WidgetComponent->SetVisibility(InEnabled);
            WidgetComponent->SetHiddenInGame(NOT InEnabled);
        }

        return InWorldSpaceWidgetHandle;
    }

    // ScreenOverlay: visibility rides RenderOpacity (legacy WorldSpaceWidgets
    // pattern) so the occlusion processor can compose enabled-state with the
    // per-frame occlusion test without fighting AddToViewport/RemoveFromParent.
    if (const auto WrapperWidget = Current.Get_WrapperWidget().Get();
        ck::IsValid(WrapperWidget))
    {
        WrapperWidget->SetRenderOpacity(InEnabled ? 1.0f : 0.0f);
    }

    return InWorldSpaceWidgetHandle;
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
