#include "CkWorldSpaceWidget_Utils.h"

#include "CkCore/Object/CkObject_Utils.h"

#include "CkEcs/EditorSelectionOwner/CkEditorSelectionOwner_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkEcsExt/SceneNode/CkSceneNode_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkUI/WorldSpaceWidget/CkWorldSpaceWidget_Fragment.h"

#include "CkUI/CkUI_Stats.h"

#include "CollisionQueryParams.h"

#include "Camera/PlayerCameraManager.h"

#include "Engine/World.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_WorldSpaceWidget_UE, FCk_Handle_WorldSpaceWidget,
    ck::FFragment_WorldSpaceWidget_Current, ck::FFragment_WorldSpaceWidget_Params);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("UI::WorldSpaceWidget OcclusionTrace"), STAT_CkUI_WSWidget_OcclusionTrace, STATGROUP_CkUI);
DECLARE_DWORD_COUNTER_STAT(TEXT("UI Occlusion Traces"), STAT_CkUI_OcclusionTraces, STATGROUP_CkUI);

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

    CK_ENSURE_IF_NOT(ck::IsValid(InAttachTo), TEXT("Cannot Create WorldSpaceWidget because the Component to attach to is INVALID"))
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

    // Request_WrapWidget already added the wrapper (which parents the content widget under its
    // ScalingBox) to the viewport, and visibility rides the wrapper's RenderOpacity. Do NOT
    // add/remove the content widget here — that re-parents it out of the wrapper.
    switch (const auto& ViewportOperation = InParams.Get_InitialViewportOperation())
    {
        case ECk_UI_Widget_ViewportOperation::DoNothing:
        case ECk_UI_Widget_ViewportOperation::RemoveFromViewport:
        {
            Current._Enabled = false;
            break;
        }
        case ECk_UI_Widget_ViewportOperation::AddToViewport:
        {
            Current._Enabled = true;
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

    CK_ENSURE_IF_NOT(ck::IsValid(World),
        TEXT("Cannot Create WorldComponent WorldSpaceWidget: invalid World for entity [{}]"), InHandle)
    { return {}; }

    const auto ContentWidget = InParams.Get_Widget().Get();
    const auto& WorldComponentInfo = InParams.Get_WorldComponentInfo();

    // DestroyOnRelease — the subsystem pins it so the fragment can hold a weak ptr
    const auto PoolParams = FCk_ObjectPooling_PoolParams{}
        .Set_RecyclePolicy(ECk_ObjectPooling_RecyclePolicy::DestroyOnRelease);

    auto ComponentOuter = static_cast<UObject*>(World);

#if WITH_EDITOR
    // Editor-preview widgets host on the per-owner proxy actor so a viewport click redirects
    // selection to the placed actor that owns the preview (see FFragment_EditorSelectionOwner).
    // World-outered widgets have no owner, so no hit proxy — click-through — outside previews.
    if (auto* SelectionProxyHost = UCk_Utils_EditorSelectionOwner_UE::TryGet_SelectionProxyHostActor(World, InHandle);
        ck::IsValid(SelectionProxyHost))
    { ComponentOuter = SelectionProxyHost; }
#endif

    auto WidgetComponent = UCk_Utils_Object_UE::Request_CreateNewObject<UWidgetComponent>(ComponentOuter,
        UWidgetComponent::StaticClass(), nullptr, PoolParams, nullptr);

    CK_ENSURE_IF_NOT(ck::IsValid(WidgetComponent),
        TEXT("Failed to create UWidgetComponent for WorldSpaceWidget"))
    { return {}; }

    WidgetComponent->SetWidgetSpace(EWidgetSpace::World);
    WidgetComponent->SetDrawSize(FVector2D{WorldComponentInfo.Get_DrawSize()});
    WidgetComponent->SetDrawAtDesiredSize(WorldComponentInfo.Get_DrawAtDesiredSize());
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

    // Explicit instance, and only after registration: the component's own InitWidget/SetWidgetClass
    // instantiation is unreliable for runtime-created components and leaves GetWidget() null.
    WidgetComponent->SetWidget(ContentWidget);

    InHandle.Add<ck::FFragment_WorldSpaceWidget_Current>(WidgetComponent, ContentWidget);

    auto TypedHandle = Cast(InHandle);

    const auto InitiallyEnabled = InParams.Get_InitialViewportOperation() == ECk_UI_Widget_ViewportOperation::AddToViewport;
    Request_SetEnabled(TypedHandle, InitiallyEnabled, {});

    return TypedHandle;
}

auto
    UCk_Utils_WorldSpaceWidget_UE::
    Request_SetEnabled(
        FCk_Handle_WorldSpaceWidget& InWorldSpaceWidgetHandle,
        bool InEnabled,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
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

        // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
        InDelegate.ExecuteIfBound(InWorldSpaceWidgetHandle, ECk_Request_OperationResult::Succeeded);
        return InWorldSpaceWidgetHandle;
    }

    if (const auto WrapperWidget = Current.Get_WrapperWidget().Get();
        ck::IsValid(WrapperWidget))
    {
        WrapperWidget->SetRenderOpacity(InEnabled ? 1.0f : 0.0f);
    }

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(InWorldSpaceWidgetHandle, ECk_Request_OperationResult::Succeeded);
    return InWorldSpaceWidgetHandle;
}

auto
    UCk_Utils_WorldSpaceWidget_UE::
    Request_SetLocationInfo(
        FCk_Handle_WorldSpaceWidget& InWorldSpaceWidgetHandle,
        const FCk_WorldSpaceWidget_LocationInfo& InLocationInfo,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_WorldSpaceWidget
{
    const auto Request = FCk_Request_WorldSpaceWidget_SetLocationInfo{InLocationInfo};

    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InWorldSpaceWidgetHandle.AddOrGet<ck::FFragment_WorldSpaceWidget_Requests>()._Requests.Emplace(Request);

    return InWorldSpaceWidgetHandle;
}

auto
    UCk_Utils_WorldSpaceWidget_UE::
    Request_SetScalingInfo(
        FCk_Handle_WorldSpaceWidget& InWorldSpaceWidgetHandle,
        const FCk_WorldSpaceWidget_ScalingInfo& InScalingInfo,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_WorldSpaceWidget
{
    const auto Request = FCk_Request_WorldSpaceWidget_SetScalingInfo{InScalingInfo};

    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InWorldSpaceWidgetHandle.AddOrGet<ck::FFragment_WorldSpaceWidget_Requests>()._Requests.Emplace(Request);

    return InWorldSpaceWidgetHandle;
}

auto
    UCk_Utils_WorldSpaceWidget_UE::
    Request_SetFadingInfo(
        FCk_Handle_WorldSpaceWidget& InWorldSpaceWidgetHandle,
        const FCk_WorldSpaceWidget_FadingInfo& InFadingInfo,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_WorldSpaceWidget
{
    const auto Request = FCk_Request_WorldSpaceWidget_SetFadingInfo{InFadingInfo};

    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InWorldSpaceWidgetHandle.AddOrGet<ck::FFragment_WorldSpaceWidget_Requests>()._Requests.Emplace(Request);

    return InWorldSpaceWidgetHandle;
}

auto
    UCk_Utils_WorldSpaceWidget_UE::
    Request_SetOcclusionInfo(
        FCk_Handle_WorldSpaceWidget& InWorldSpaceWidgetHandle,
        const FCk_WorldSpaceWidget_OcclusionInfo& InOcclusionInfo,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_WorldSpaceWidget
{
    const auto Request = FCk_Request_WorldSpaceWidget_SetOcclusionInfo{InOcclusionInfo};

    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InWorldSpaceWidgetHandle.AddOrGet<ck::FFragment_WorldSpaceWidget_Requests>()._Requests.Emplace(Request);

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

auto
    UCk_Utils_WorldSpaceWidget_UE::
    Get_IsAnchorOccluded(
        const FCk_Handle_WorldSpaceWidget& InWorldSpaceWidgetHandle)
    -> bool
{
    SCOPE_CYCLE_COUNTER(STAT_CkUI_WSWidget_OcclusionTrace);
    INC_DWORD_STAT(STAT_CkUI_OcclusionTraces);

    if (ck::Is_NOT_Valid(InWorldSpaceWidgetHandle))
    { return false; }

    const auto& Params = InWorldSpaceWidgetHandle.Get<ck::FFragment_WorldSpaceWidget_Params>();
    const auto& Current = InWorldSpaceWidgetHandle.Get<ck::FFragment_WorldSpaceWidget_Current>();

    const auto PlayerController = Current.Get_WidgetOwningPlayer().Get();
    if (ck::Is_NOT_Valid(PlayerController))
    { return false; }

    const auto CameraManager = PlayerController->PlayerCameraManager;
    if (ck::Is_NOT_Valid(CameraManager))
    { return false; }

    const auto World = PlayerController->GetWorld();
    if (ck::Is_NOT_Valid(World))
    { return false; }

    const auto& WidgetTransform = InWorldSpaceWidgetHandle.Get<ck::FFragment_Transform>().Get_Transform().GetLocation();
    const auto& WidgetOffset = Params.Get_LocationInfo().Get_WorldSpaceOffset();

    const auto AnchorWorldLocation = WidgetTransform + WidgetOffset;

    auto QueryParams = FCollisionQueryParams{FName{TEXT("CkWorldSpaceWidgetOcclusion")}, true};
    if (const auto PlayerPawn = PlayerController->GetPawn();
        ck::IsValid(PlayerPawn))
    { QueryParams.AddIgnoredActor(PlayerPawn); }

    auto Hit = FHitResult{};
    World->LineTraceSingleByChannel(
        Hit,
        CameraManager->GetCameraLocation(),
        AnchorWorldLocation,
        Params.Get_OcclusionInfo().Get_TraceChannel().GetValue(),
        QueryParams);

    return Hit.IsValidBlockingHit();
}

// --------------------------------------------------------------------------------------------------------------------
