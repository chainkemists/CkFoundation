#include "CkPmg_Utils_Donut.h"

#include "CkPmg/CkPmg_Log.h"
#include "CkPmg_Fragment.h"

#include "CkEcs/EditorSelectionOwner/CkEditorSelectionOwner_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------
// Pmg Donut Utils
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_Donut_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Pmg_Donut_Spec& InParams)
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

auto
    UCk_Utils_Pmg_Donut_UE::
    Create(
        FCk_Handle& InOwner,
        const FCk_Pmg_Donut_Spec& InParams)
    -> FCk_Handle_Pmg_Donut
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwner);
    return Add(NewEntity, InParams);
}

auto
    UCk_Utils_Pmg_Donut_UE::
    Request_ActAsEditorSelectionHandle(
        FCk_Handle_Pmg_Donut& InDonut)
    -> FCk_Handle_Pmg_Donut
{
    InDonut.AddOrGet<ck::FTag_Pmg_EditorSelectionHandle>();
    return InDonut;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_Donut_UE::
    Get_MeshComponentOuter(
        UWorld* InWorld,
        const FCk_Handle_Pmg_Donut& InDonut)
    -> UObject*
{
#if WITH_EDITOR
    if (ck::IsValid(InWorld) && InWorld->WorldType == EWorldType::Editor)
    {
        // Donuts are standalone shapes (no composite children) — a direct tag check suffices.
        if (InDonut.Has<ck::FTag_Pmg_EditorSelectionHandle>())
        {
            if (auto* SelectionProxyHost = UCk_Utils_EditorSelectionOwner_UE::TryGet_SelectionProxyHostActor(InWorld, InDonut);
                ck::IsValid(SelectionProxyHost))
            { return SelectionProxyHost; }
        }
    }
#endif

    return InWorld;
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Pmg_Donut_UE, FCk_Handle_Pmg_Donut,
    ck::FFragment_Pmg_Donut_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_Donut_UE::
    Request_UpdateParams(
        FCk_Handle_Pmg_Donut& InDonut,
        const FCk_Request_Pmg_Donut_UpdateParams& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Pmg_Donut
{
    CK_CALLSTACK_RECORD(ck::FFragment_Pmg_Donut_UpdateParams, InDonut);

    ck::pmg::Verbose(TEXT("Requesting param update for Pmg Donut [{}]"), InDonut);

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

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
