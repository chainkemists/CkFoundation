#include "CkPmg_Utils_DebugShapes.h"

#include "CkPmg_Fragment.h"
#include "CkPmg/CkPmg_Fragment_TextShapes.h"

#include "CkEcs/EditorSelectionOwner/CkEditorSelectionOwner_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Request_ActAsEditorSelectionHandle(
        FCk_Handle_Pmg_DebugShape& InDebugShape)
    -> FCk_Handle_Pmg_DebugShape
{
    InDebugShape.AddOrGet<ck::FTag_Pmg_EditorSelectionHandle>();
    return InDebugShape;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Get_MeshComponentOuter(
        UWorld* InWorld,
        const FCk_Handle_Pmg_DebugShape& InShape)
    -> UObject*
{
#if WITH_EDITOR
    if (ck::IsValid(InWorld) && InWorld->WorldType == EWorldType::Editor)
    {
        // Composite shapes render through child entities, so honor a tag anywhere up the chain.
        const auto TaggedEntity = UCk_Utils_EntityLifetime_UE::Get_EntityInOwnershipChain_If(InShape,
            [](const FCk_Handle& InEntityInChain)
            {
                return InEntityInChain.Has<ck::FTag_Pmg_EditorSelectionHandle>();
            });

        if (ck::IsValid(TaggedEntity))
        {
            if (auto* SelectionProxyHost = UCk_Utils_EditorSelectionOwner_UE::TryGet_SelectionProxyHostActor(InWorld, InShape);
                ck::IsValid(SelectionProxyHost))
            { return SelectionProxyHost; }
        }
    }
#endif

    return InWorld;
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Pmg_DebugShape_UE, FCk_Handle_Pmg_DebugShape,
    ck::FFragment_Pmg_DebugShape_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Request_SetColor(
        FCk_Handle_Pmg_DebugShape& InHandle,
        const FCk_Request_Pmg_DebugShape_SetColor& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Pmg_DebugShape
{
    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InHandle.AddOrGet<ck::FFragment_Pmg_DebugShape_Requests>()._Requests.Emplace(InRequest);
    return InHandle;
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Request_SetText(
        FCk_Handle_Pmg_DebugShape& InHandle,
        FString InNewText,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Pmg_DebugShape
{
    const auto IsTextShape = InHandle.Has<ck::FFragment_Pmg_Text_Params>();
    CK_ENSURE_IF_NOT(IsTextShape,
        TEXT("Request_SetText called on a non-text PMG shape [{}]"), InHandle)
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHandle;
    }

    InHandle.Get<ck::FFragment_Pmg_Text_Params>().Set_Text(InNewText);
    InHandle.AddOrGet<ck::FTag_Pmg_DebugShape_NeedsSetup>();

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Succeeded);
    return InHandle;
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Request_SetLineThickness(
        FCk_Handle_Pmg_DebugShape& InHandle,
        const FCk_Request_Pmg_DebugShape_SetLineThickness& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Pmg_DebugShape
{
    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InHandle.AddOrGet<ck::FFragment_Pmg_DebugShape_Requests>()._Requests.Emplace(InRequest);
    return InHandle;
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Request_SetDrawLines(
        FCk_Handle_Pmg_DebugShape& InHandle,
        const FCk_Request_Pmg_DebugShape_SetDrawLines& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Pmg_DebugShape
{
    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InHandle.AddOrGet<ck::FFragment_Pmg_DebugShape_Requests>()._Requests.Emplace(InRequest);
    return InHandle;
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Request_SetDuration(
        FCk_Handle_Pmg_DebugShape& InHandle,
        const FCk_Request_Pmg_DebugShape_SetDuration& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Pmg_DebugShape
{
    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InHandle.AddOrGet<ck::FFragment_Pmg_DebugShape_Requests>()._Requests.Emplace(InRequest);
    return InHandle;
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Request_SetRenderMode(
        FCk_Handle_Pmg_DebugShape& InHandle,
        const FCk_Request_Pmg_DebugShape_SetRenderMode& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Pmg_DebugShape
{
    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InHandle.AddOrGet<ck::FFragment_Pmg_DebugShape_Requests>()._Requests.Emplace(InRequest);
    return InHandle;
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Request_SetVisible(
        FCk_Handle_Pmg_DebugShape& InHandle,
        bool InIsVisible,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Pmg_DebugShape
{
    const auto NewMode = InIsVisible
        ? ECk_Pmg_RenderMode::DoubleSided
        : ECk_Pmg_RenderMode::Hidden;
    return Request_SetRenderMode(InHandle, FCk_Request_Pmg_DebugShape_SetRenderMode{NewMode}, InDelegate);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Request_SetEnableCollision(
        FCk_Handle_Pmg_DebugShape& InHandle,
        const FCk_Request_Pmg_DebugShape_SetEnableCollision& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Pmg_DebugShape
{
    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InHandle.AddOrGet<ck::FFragment_Pmg_DebugShape_Requests>()._Requests.Emplace(InRequest);
    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------
