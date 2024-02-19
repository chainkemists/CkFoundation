#include "CkRenderStatus_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkGraphics/RenderStatus/CkRenderStatus_Fragment.h"

#include "CkVariables/CkUnrealVariables_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_RenderStatus_UE::
    Add(
        FCk_Handle_UnderConstruction& InHandle,
        const FCk_Fragment_RenderStatus_ParamsData& InParams)
    -> void
{
    InHandle.Add<ck::FFragment_RenderStatus_Params>(InParams);

    switch(const auto& RenderGroup = InParams.Get_RenderGroup())
    {
        case ECk_RenderStatus_Group::Unspecified:
        {
            break;
        }
        case ECk_RenderStatus_Group::WorldStatic:
        {
            InHandle.Add<ck::FTag_RenderStatus_Group_WorldStatic>();
            break;
        }
        case ECk_RenderStatus_Group::WorldDynamic:
        {
            InHandle.Add<ck::FTag_RenderStatus_Group_WorldDynamic>();
            break;
        }
        case ECk_RenderStatus_Group::Pawn:
        {
            InHandle.Add<ck::FTag_RenderStatus_Group_Pawn>();
            break;
        }
        default:
        {
            CK_INVALID_ENUM(RenderGroup);
        };
    }
}

auto
    UCk_Utils_RenderStatus_UE::
    Has(
        FCk_Handle InHandle)
    -> bool
{
    return InHandle.Has<ck::FFragment_RenderStatus_Params>();
}

auto
    UCk_Utils_RenderStatus_UE::
    Ensure(
        FCk_Handle InHandle)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InHandle), TEXT("Handle [{}] does NOT have RenderStatus"), InHandle)
    { return false; }

    return true;
}

auto
    UCk_Utils_RenderStatus_UE::
    Request_QueryRenderedActors(
        FCk_Handle& InHandle,
        const FCk_Request_RenderStatus_QueryRenderedActors& InRequest,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Delegate_RenderStatus_OnRenderedActorsQueried& InDelegate)
    -> void
{
    auto RequestEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);

    RequestEntity.AddOrGet<ck::FFragment_RenderStatus_Requests>()._Requests.Add(InRequest);
    UCk_Utils_Variables_InstancedStruct_UE::Set(RequestEntity, FGameplayTag::EmptyTag, InOptionalPayload);
    ck::UUtils_Signal_OnRenderedActorsQueried_PostFireUnbind::Bind(RequestEntity, InDelegate, ECk_Signal_BindingPolicy::FireIfPayloadInFlight);
}

// --------------------------------------------------------------------------------------------------------------------
