#include "CkShapeCapsule_Utils.h"

#include "CkShapes/CkShapes_Log.h"
#include "CkShapes/CkShapes_Utils.h"
#include "CkShapes/Capsule/CkShapeCapsule_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ShapeCapsule_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_ShapeCapsule_ParamsData& InParams)
    -> FCk_Handle_ShapeCapsule
{
    CK_ENSURE_IF_NOT(NOT UCk_Utils_Shapes_UE::Has(InHandle),
        TEXT("Trying to Add a Capsule Shape to [{}] but it already has an existing Shape feature!"), InHandle)
    { return {}; }

    InHandle.Add<ck::FFragment_ShapeCapsule_Params>(InParams);
    InHandle.Add<ck::FFragment_ShapeCapsule_Current>(InParams.Get_InitialDimensions());

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_ShapeCapsule_UE, FCk_Handle_ShapeCapsule,
    ck::FFragment_ShapeCapsule_Params, ck::FFragment_ShapeCapsule_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ShapeCapsule_UE::
    Request_UpdateDimensions(
        FCk_Handle_ShapeCapsule& InShapeCapsule,
        const FCk_Request_ShapeCapsule_UpdateDimensions& InRequest)
    -> FCk_Handle_ShapeCapsule
{
    InShapeCapsule.AddOrGet<ck::FFragment_ShapeCapsule_Requests>()._Requests.Emplace(InRequest);
    return InShapeCapsule;
}

auto
    UCk_Utils_ShapeCapsule_UE::
    Get_Dimensions(
        const FCk_Handle_ShapeCapsule& InShapeCapsule)
        -> FCk_ShapeCapsule_Dimensions
{
    return InShapeCapsule.Get<ck::FFragment_ShapeCapsule_Current>().Get_Dimensions();
}

auto
    UCk_Utils_ShapeCapsule_UE::
    BindTo_OnDimensionsChanged(
        FCk_Handle_ShapeCapsule& InShapeCapsule,
        const FCk_Delegate_ShapeCapsule_OnDimensionsChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_ShapeCapsule
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnShapeCapsuleDimensionsChanged, InShapeCapsule, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InShapeCapsule;
}

auto
    UCk_Utils_ShapeCapsule_UE::
    UnbindFrom_OnDimensionsChanged(
        FCk_Handle_ShapeCapsule& InShapeCapsule,
        const FCk_Delegate_ShapeCapsule_OnDimensionsChanged& InDelegate)
    -> FCk_Handle_ShapeCapsule
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnShapeCapsuleDimensionsChanged, InShapeCapsule, InDelegate);
    return InShapeCapsule;
}

// --------------------------------------------------------------------------------------------------------------------
