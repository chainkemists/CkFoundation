#include "CkIsmRenderer_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Math/Vector/CkVector_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkNet/CkNet_Utils.h"

#include <Components/InstancedStaticMeshComponent.h>
#include <Components/HierarchicalInstancedStaticMeshComponent.h>
#include <Misc/EnumRange.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_IsmRenderer_Setup::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_IsmRenderer_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmRenderer_Params& InParams,
            const FFragment_OwningActor_Current& InOwningActorCurrent) const
        -> void
    {
        const auto& Actor = InOwningActorCurrent.Get_EntityOwningActor().Get();
        const auto& Params = InParams.Get_Params();

        CK_ENSURE_IF_NOT(ck::IsValid(Actor),
            TEXT("OwningActor [{}] for Handle [{}] is INVALID. Unable to Setup the IsmRenderer"), Actor, InHandle)
        { return; }

        CK_ENSURE_IF_NOT(ck::IsValid(Params.Get_Mesh()),
            TEXT("The Mesh [{}] is INVALID. Unable to Setup the IsmRenderer"), InParams.Get_Params().Get_Mesh())
        { return; }

        CK_ENSURE_IF_NOT(ck::IsValid(Params.Get_RendererName()),
            TEXT("The Renderer Name [{}] is INVALID. Unable to Setup the IsmRenderer"), Params.Get_RendererName())
        { return; }

        const auto AddIsmActorComponents = [&]<typename T_IsmCompTypeTag>(T_IsmCompTypeTag)
        {
            using T_IsmCompType = typename T_IsmCompTypeTag::type;

            for (auto Mobility : TEnumRange<ECk_Mobility>())
            {
                UCk_Utils_Actor_UE::Request_AddNewActorComponent<T_IsmCompType>
                (
                    UCk_Utils_Actor_UE::AddNewActorComponent_Params<T_IsmCompType>{Actor}.Set_IsUnique(false),
                    IsmActorComponentInitFunctor<T_IsmCompType>{InHandle, InParams, Mobility}
                );
            }
        };

        switch (const auto& RenderPolicy = Params.Get_RenderPolicy())
        {
            case ECk_Ism_RenderPolicy::ISM:
            {
                AddIsmActorComponents(std::type_identity<UInstancedStaticMeshComponent>{});
                break;
            }
            case ECk_Ism_RenderPolicy::HISM:
            {
                AddIsmActorComponents(std::type_identity<UHierarchicalInstancedStaticMeshComponent>{});
                break;
            }
            default:
            {
                CK_INVALID_ENUM(RenderPolicy);
                break;
            }
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IsmRenderer_ClearInstances::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmRenderer_Current& InCurrent) const
        -> void
    {
        if (ck::Is_NOT_Valid(InCurrent.Get_IsmComponent_Movable()))
        { return; }

        InCurrent.Get_IsmComponent_Movable()->ClearInstances();
    }
}

// --------------------------------------------------------------------------------------------------------------------
