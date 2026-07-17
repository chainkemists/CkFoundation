#include "CkPmg_Utils_DebugShapes.h"

#include "CkPmg_Fragment.h"
#include "CkPmg/CkPmg_Fragment_TextShapes.h"

#include "CkEcs/EditorSelectionOwner/CkEditorSelectionOwner_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------
// Pmg DebugShape Utils - Generic Infrastructure
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
        // Composite shapes (icons, dashed lines, text glyph runs) render through child
        // entities — honor an opt-in stamped anywhere up the lifetime chain.
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
        const FCk_Request_Pmg_DebugShape_SetColor& InRequest)
    -> FCk_Handle_Pmg_DebugShape
{
    InHandle.AddOrGet<ck::FFragment_Pmg_DebugShape_Requests>()._Requests.Emplace(InRequest);
    return InHandle;
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Request_SetText(
        FCk_Handle_Pmg_DebugShape& InHandle,
        FString InNewText)
    -> FCk_Handle_Pmg_DebugShape
{
    CK_ENSURE_IF_NOT(InHandle.Has<ck::FFragment_Pmg_Text_Params>(),
        TEXT("Request_SetText called on a non-text PMG shape [{}]"), InHandle)
    { return InHandle; }

    InHandle.Get<ck::FFragment_Pmg_Text_Params>().Set_Text(InNewText);
    InHandle.AddOrGet<ck::FTag_Pmg_DebugShape_NeedsSetup>();
    return InHandle;
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Request_SetLineThickness(
        FCk_Handle_Pmg_DebugShape& InHandle,
        const FCk_Request_Pmg_DebugShape_SetLineThickness& InRequest)
    -> FCk_Handle_Pmg_DebugShape
{
    InHandle.AddOrGet<ck::FFragment_Pmg_DebugShape_Requests>()._Requests.Emplace(InRequest);
    return InHandle;
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Request_SetDrawLines(
        FCk_Handle_Pmg_DebugShape& InHandle,
        const FCk_Request_Pmg_DebugShape_SetDrawLines& InRequest)
    -> FCk_Handle_Pmg_DebugShape
{
    InHandle.AddOrGet<ck::FFragment_Pmg_DebugShape_Requests>()._Requests.Emplace(InRequest);
    return InHandle;
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Request_SetDuration(
        FCk_Handle_Pmg_DebugShape& InHandle,
        const FCk_Request_Pmg_DebugShape_SetDuration& InRequest)
    -> FCk_Handle_Pmg_DebugShape
{
    InHandle.AddOrGet<ck::FFragment_Pmg_DebugShape_Requests>()._Requests.Emplace(InRequest);
    return InHandle;
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Request_SetRenderMode(
        FCk_Handle_Pmg_DebugShape& InHandle,
        const FCk_Request_Pmg_DebugShape_SetRenderMode& InRequest)
    -> FCk_Handle_Pmg_DebugShape
{
    InHandle.AddOrGet<ck::FFragment_Pmg_DebugShape_Requests>()._Requests.Emplace(InRequest);
    return InHandle;
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Request_SetVisible(
        FCk_Handle_Pmg_DebugShape& InHandle,
        bool InIsVisible)
    -> FCk_Handle_Pmg_DebugShape
{
    // Routes through Request_SetRenderMode so the existing handler in
    // FProcessor_Pmg_DebugShape_HandleRequests does the procmesh visibility flip; the
    // DrawLines processor reads RenderMode == Hidden to skip wireframe re-emission.
    // Visible callers fall back to DoubleSided (the framework's default visible mode); a
    // caller that wants SingleSided specifically should use Request_SetRenderMode directly.
    const auto NewMode = InIsVisible
        ? ECk_Pmg_RenderMode::DoubleSided
        : ECk_Pmg_RenderMode::Hidden;
    return Request_SetRenderMode(InHandle, FCk_Request_Pmg_DebugShape_SetRenderMode{NewMode});
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Request_SetEnableCollision(
        FCk_Handle_Pmg_DebugShape& InHandle,
        const FCk_Request_Pmg_DebugShape_SetEnableCollision& InRequest)
    -> FCk_Handle_Pmg_DebugShape
{
    InHandle.AddOrGet<ck::FFragment_Pmg_DebugShape_Requests>()._Requests.Emplace(InRequest);
    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------
