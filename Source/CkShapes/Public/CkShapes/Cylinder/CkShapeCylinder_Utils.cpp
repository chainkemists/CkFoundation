#include "CkShapeCylinder_Utils.h"

#include "CkShapes/CkShapes_Log.h"
#include "CkShapes/CkShapes_Utils.h"
#include "CkShapes/Cylinder/CkShapeCylinder_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ShapeCylinder_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_ShapeCylinder_ParamsData& InParams)
    -> FCk_Handle_ShapeCylinder
{
    CK_ENSURE_IF_NOT(NOT UCk_Utils_Shapes_UE::Has(InHandle),
        TEXT("Trying to Add a Cylinder Shape to [{}] but it already has an existing Shape feature!"), InHandle)
    { return {}; }

    InHandle.Add<ck::FFragment_ShapeCylinder_Params>(InParams);
    InHandle.Add<ck::FFragment_ShapeCylinder_Current>(InParams.Get_InitialDimensions());

    return Cast(InHandle);
}

auto
    UCk_Utils_ShapeCylinder_UE::
    Create(
        FCk_Handle& InOwner,
        const FCk_Fragment_ShapeCylinder_ParamsData& InParams)
    -> FCk_Handle_ShapeCylinder
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwner);
    return Add(NewEntity, InParams);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_ShapeCylinder_UE, FCk_Handle_ShapeCylinder,
    ck::FFragment_ShapeCylinder_Params, ck::FFragment_ShapeCylinder_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ShapeCylinder_UE::
    Request_UpdateDimensions(
        FCk_Handle_ShapeCylinder& InShapeCylinder,
        const FCk_Request_ShapeCylinder_UpdateDimensions& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_ShapeCylinder
{
    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InShapeCylinder.AddOrGet<ck::FFragment_ShapeCylinder_Requests>()._Requests.Emplace(InRequest);
    return InShapeCylinder;
}

auto
    UCk_Utils_ShapeCylinder_UE::
    Get_Dimensions(
        const FCk_Handle_ShapeCylinder& InShapeCylinder)
        -> FCk_ShapeCylinder_Dimensions
{
    return InShapeCylinder.Get<ck::FFragment_ShapeCylinder_Current>().Get_Dimensions();
}

auto
    UCk_Utils_ShapeCylinder_UE::
    BindTo_OnDimensionsChanged(
        FCk_Handle_ShapeCylinder& InShapeCylinder,
        const FCk_Delegate_ShapeCylinder_OnDimensionsChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_ShapeCylinder
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnShapeCylinderDimensionsChanged, InShapeCylinder, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InShapeCylinder;
}

auto
    UCk_Utils_ShapeCylinder_UE::
    UnbindFrom_OnDimensionsChanged(
        FCk_Handle_ShapeCylinder& InShapeCylinder,
        const FCk_Delegate_ShapeCylinder_OnDimensionsChanged& InDelegate)
    -> FCk_Handle_ShapeCylinder
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnShapeCylinderDimensionsChanged, InShapeCylinder, InDelegate);
    return InShapeCylinder;
}

// --------------------------------------------------------------------------------------------------------------------
