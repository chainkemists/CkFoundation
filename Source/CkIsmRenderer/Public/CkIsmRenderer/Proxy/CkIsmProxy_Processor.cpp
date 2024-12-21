#include "CkIsmProxy_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Math/Vector/CkVector_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkIsmRenderer/Renderer/CkIsmRenderer_Fragment.h"

#include "CkNet/CkNet_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_sim_renderer_processor
{
    using IsmAcArray = std::array<TWeakObjectPtr<UInstancedStaticMeshComponent>,  static_cast<int32>(ECk_Mobility::Count)>;
    static TMap<FGameplayTag, IsmAcArray> Renderers;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_IsmProxy_Static::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        using ck_sim_renderer_processor::Renderers;

        _TransientEntity.View<
            FFragment_IsmRenderer_Params,
            FFragment_IsmRenderer_Current,
            TExclude<FTag_IsmRenderer_NeedsSetup>>().ForEach(
            [&](FCk_Entity InHandle, const FFragment_IsmRenderer_Params& InParams, const FFragment_IsmRenderer_Current& InCurrent)
            {
                using ck_sim_renderer_processor::IsmAcArray;

                const auto IsmArray = IsmAcArray{
                    InCurrent.Get_IsmComponent_Static(), InCurrent.Get_IsmComponent_Static(), InCurrent.Get_IsmComponent_Movable()};

                Renderers.Add(InParams.Get_Params().Get_RendererName(), IsmArray);
            });

        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_IsmProxy_Static::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_Params& InParams) const
        -> void
    {
        using ck_sim_renderer_processor::Renderers;

        const auto& Params = InParams.Get_Params();
        const auto& RendererName = Params.Get_RendererName();
        const auto& RelativeTransform = [&]
        {
            const auto& ProxyRelativeTransform = Params.Get_RelativeTransform();
            const auto& ProxyRelativeScale = ProxyRelativeTransform.GetScale3D();

            CK_ENSURE_IF_NOT(NOT UCk_Utils_Vector3_UE::Get_IsAnyAxisNearlyZero(ProxyRelativeScale),
                TEXT("IsmProxy Relative Scale has one or more axis nearly equal to 0. Setting it to 1 in non-shipping build"), ProxyRelativeScale)
            { return FTransform{ ProxyRelativeTransform.GetRotation(), ProxyRelativeTransform.GetLocation(), FVector::OneVector }; }

            return ProxyRelativeTransform;
        }();

        const auto& MaybeFound = Renderers.Find(RendererName);

        CK_ENSURE_IF_NOT(ck::IsValid(MaybeFound, ck::IsValid_Policy_NullptrOnly{}),
            TEXT("No Renderer with Name [{}] found. Unable to Setup the IsmProxy"), RendererName)
        { return; }

        const auto& RendererToUse = *MaybeFound;

        CK_ENSURE_IF_NOT(ck::IsValid(RendererToUse[0]) && ck::IsValid(RendererToUse[1]) && ck::IsValid(RendererToUse[2]),
            TEXT("(One of the) Renderer(s) with Name [{}] is INVALID. Unable to Setup the IsmProxy"), RendererName)
        { return; }

        CK_ENSURE_IF_NOT(UCk_Utils_Transform_UE::Has(InHandle),
            TEXT("Ism Proxy Entity [{}] does NOT have the required Transform feature. Unable to Setup the IsmProxy"), InHandle)
        { return; }

        const auto& CurrentTransform = UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentTransform(InHandle);
        const auto& CombinedTransform = CurrentTransform * RelativeTransform;

        const auto& MeshMobility = InHandle.Has<FTag_IsmProxy_Dynamic>() ? ECk_Mobility::Movable : ECk_Mobility::Static;

        RendererToUse[static_cast<int32>(MeshMobility)]->AddInstance(CombinedTransform);

        InHandle.Remove<MarkedDirtyBy>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_IsmProxy_Dynamic::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IsmProxy_Params& InParams) const
        -> void
    {
        using ck_sim_renderer_processor::Renderers;

        const auto& Params = InParams.Get_Params();
        const auto& RendererName = Params.Get_RendererName();
        const auto& RelativeTransform = [&]
        {
            const auto& ProxyRelativeTransform = Params.Get_RelativeTransform();
            const auto& ProxyRelativeScale = ProxyRelativeTransform.GetScale3D();

            CK_ENSURE_IF_NOT(NOT UCk_Utils_Vector3_UE::Get_IsAnyAxisNearlyZero(ProxyRelativeScale),
                TEXT("IsmProxy Relative Scale has one or more axis nearly equal to 0. Setting it to 1 in non-shipping build"), ProxyRelativeScale)
            { return FTransform{ ProxyRelativeTransform.GetRotation(), ProxyRelativeTransform.GetLocation(), FVector::OneVector }; }

            return ProxyRelativeTransform;
        }();

        const auto& MaybeFound = Renderers.Find(RendererName);

        CK_ENSURE_IF_NOT(ck::IsValid(MaybeFound, ck::IsValid_Policy_NullptrOnly{}),
            TEXT("No Renderer with Name [{}] found. Unable to Setup the IsmProxy"), RendererName)
        { return; }

        const auto& RendererToUse = *MaybeFound;

        CK_ENSURE_IF_NOT(ck::IsValid(RendererToUse[static_cast<int32>(ECk_Mobility::Movable)]),
            TEXT("The MOVABLE Renderer with Name [{}] is INVALID. Unable to Render the MOVABLE IsmProxy"), RendererName)
        { return; }

        CK_ENSURE_IF_NOT(UCk_Utils_Transform_UE::Has(InHandle),
            TEXT("Ism Proxy Entity [{}] does NOT have the required Transform feature. Unable to Setup the IsmProxy"), InHandle)
        { return; }

        const auto& CurrentTransform = UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentTransform(InHandle);
        const auto& CombinedTransform = CurrentTransform * RelativeTransform;

        RendererToUse[static_cast<int32>(ECk_Mobility::Movable)]->AddInstance(CombinedTransform);
    }
}

// --------------------------------------------------------------------------------------------------------------------
