#include "CkRenderProxy_Processor.h"

#include "NaniteSceneProxy.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkRenderProxy/CkRenderProxy_Log.h"
#include "CkRenderProxy/Manager/CkRenderProxyManager_Subsystem.h"
#include "CkRenderProxy/Proxy/CkRenderProxy_Utils.h"

#include <StaticMeshResources.h>
#include <Engine/StaticMesh.h>
#include <PrimitiveSceneProxy.h>
#include "RHICommandList.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_RenderProxy_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_RenderProxy_Params& InParams,
            FFragment_RenderProxy_Current& InCurrent,
            const FFragment_Transform& InTransform) const
        -> void
    {
        InHandle.Remove<MarkedDirtyBy>();

        const auto& Mesh = InParams.Get_Mesh();

        CK_ENSURE_IF_NOT(ck::IsValid(Mesh),
            TEXT("RenderProxy [{}] has invalid mesh. Cannot setup."), InHandle)
        { return; }

        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);

        CK_ENSURE_IF_NOT(ck::IsValid(World),
            TEXT("RenderProxy [{}] has invalid world. Cannot setup."), InHandle)
        { return; }

        const auto& Subsystem = World->GetSubsystem<UCk_RenderProxyManager_Subsystem>();

        CK_ENSURE_IF_NOT(ck::IsValid(Subsystem),
            TEXT("RenderProxy subsystem not found for [{}]. Cannot setup."), InHandle)
        { return; }

        const auto& ManagerComponent = Subsystem->Get_ManagerComponent();

        CK_ENSURE_IF_NOT(ck::IsValid(ManagerComponent),
            TEXT("RenderProxy manager component not found for [{}]. Cannot setup."), InHandle)
        { return; }

        // Generate unique instance ID for editor tracking
        InCurrent._InstanceId = FGuid::NewGuid();

        // Create render proxy data
        InCurrent._Data = MakeUnique<FRenderProxyData>();
        auto& Data = *InCurrent._Data;

        // Setup proxy descriptor
        Data.ProxyDesc.StaticMesh = Mesh;
        Data.ProxyDesc.CustomPrimitiveData = &Data.CustomData;
        Data.ProxyDesc.NaniteResources = Mesh->GetRenderData()->NaniteResourcesPtr.Get();
        Data.ProxyDesc.World = World;
        Data.ProxyDesc.Owner = ManagerComponent;
        Data.ProxyDesc.Scene = World->Scene;
        Data.ProxyDesc.ComponentId = Data.SceneInfoData.PrimitiveSceneId;

        // Create the scene proxy
        Nanite::FMaterialAudit NaniteMaterials{};
        if (Data.ProxyDesc.ShouldCreateNaniteProxy(&NaniteMaterials))
        {
            InCurrent._Proxy = new Nanite::FSceneProxy(NaniteMaterials, Data.ProxyDesc);
        }
        else
        {
            InCurrent._Proxy = new FStaticMeshSceneProxy(Data.ProxyDesc, false);
        }

        Data.SceneInfoData.SceneProxy = InCurrent._Proxy;

        // Setup scene descriptor
        Data.SceneDesc.World = World;
        Data.SceneDesc.PrimitiveUObject = ManagerComponent;
        Data.SceneDesc.ProxyDesc = &Data.ProxyDesc;
        Data.SceneDesc.PrimitiveSceneData = &Data.SceneInfoData;
        Data.SceneDesc.LocalBounds = Mesh->GetBounds();

        // Set initial transform
        const auto& Transform = InTransform.Get_Transform();
        Data.SceneDesc.RenderMatrix = Transform.ToMatrixWithScale();
        Data.SceneDesc.AttachmentRootPosition = Transform.GetLocation();
        Data.SceneDesc.Bounds = Data.SceneDesc.LocalBounds.TransformBy(Transform);

        InCurrent._CachedBounds = Data.SceneDesc.Bounds;

        // Add to scene
        if (InParams.Get_StartingState() == ECk_EnableDisable::Enable)
        {
            World->Scene->AddPrimitive(&Data.SceneDesc);
        }
        else
        {
            InHandle.AddOrGet<FTag_RenderProxy_Disabled>();
        }

        // Setup mobility tags
        if (InParams.Get_Mobility() == ECk_Mobility::Movable)
        {
            InHandle.AddOrGet<FTag_RenderProxy_Movable>();
        }

        // Register with manager for editor selection
        ManagerComponent->ProxyToEntityMap.Add(InCurrent.Get_InstanceId(), InHandle);

        render_proxy::Verbose(TEXT("Setup RenderProxy [{}] for mesh [{}]"), InHandle, Mesh->GetName());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_RenderProxy_UpdateTransform::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        _WorldsToMarkDirty.Reset();

        TProcessor::DoTick(InDeltaT);

        // Mark all affected worlds' render state dirty
        for (auto World : _WorldsToMarkDirty)
        {
            if (ck::IsValid(World))
            {
                auto Scene = World->Scene;
                ENQUEUE_RENDER_COMMAND(UpdateAllPrimitiveSceneInfosCmd)([Scene](FRHICommandListImmediate& RHICmdList)
                {
                    Scene->UpdateAllPrimitiveSceneInfos(RHICmdList);
                });
            }
        }
    }

    auto
        FProcessor_RenderProxy_UpdateTransform::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_RenderProxy_Current& InCurrent,
            const FFragment_Transform& InTransform)
        -> void
    {
        if (ck::Is_NOT_Valid(InCurrent.Get_Data()))
        { return; }

        auto& Data = *InCurrent._Data;
        const auto& Transform = InTransform.Get_Transform();

        // Update transform
        Data.SceneDesc.RenderMatrix = Transform.ToMatrixWithScale();
        Data.SceneDesc.AttachmentRootPosition = Transform.GetLocation();
        Data.SceneDesc.Bounds = Data.SceneDesc.LocalBounds.TransformBy(Transform);

        InCurrent._CachedBounds = Data.SceneDesc.Bounds;

        // Track world for batch update
        if (ck::IsValid(Data.SceneDesc.World))
        {
            _WorldsToMarkDirty.Add(Data.SceneDesc.World);
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_RenderProxy_EnsureStaticNotMoved_DEBUG::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_RenderProxy_Params& InParams)
        -> void
    {
        CK_TRIGGER_ENSURE(TEXT("RenderProxy [{}] with Mobility [{}] had its Transform changed.\n"
                "If this RenderProxy is meant to move its Mobility shouldn't be [{}]"),
            InHandle,
            InParams.Get_Mobility(),
            InParams.Get_Mobility());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_RenderProxy_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_RenderProxy_Current& InCurrent,
            const FFragment_RenderProxy_Requests& InRequestsComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestsComp,
        [&](FFragment_RenderProxy_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests.Get_Requests(), Visitor(
            [&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, InCurrent, InRequest);

                if (InRequest.Get_IsRequestHandleValid())
                {
                    InRequest.GetAndDestroyRequestHandle();
                }
            }));
        });
    }

    auto
        FProcessor_RenderProxy_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_RenderProxy_Current& InCurrent,
            const FCk_Request_RenderProxy_EnableDisable& InRequest) const
        -> void
    {
        if (ck::Is_NOT_Valid(InCurrent.Get_Data()))
        { return; }

        auto& Data = *InCurrent._Data;

        switch (InRequest.Get_EnableDisable())
        {
            case ECk_EnableDisable::Enable:
            {
                if (InHandle.Try_Remove<FTag_RenderProxy_Disabled>() == 0)
                { return; }

                if (ck::IsValid(Data.SceneDesc.World) && ck::IsValid(Data.SceneDesc.World->Scene, ck::IsValid_Policy_NullptrOnly{}))
                {
                    Data.SceneDesc.World->Scene->AddPrimitive(&Data.SceneDesc);
                }

                break;
            }
            case ECk_EnableDisable::Disable:
            {
                if (InHandle.Has<FTag_RenderProxy_Disabled>())
                { return; }

                InHandle.AddOrGet<FTag_RenderProxy_Disabled>();

                if (ck::IsValid(Data.SceneDesc.World) && ck::IsValid(Data.SceneDesc.World->Scene, ck::IsValid_Policy_NullptrOnly{}))
                {
                    Data.SceneDesc.World->Scene->RemovePrimitive(&Data.SceneDesc);
                }

                break;
            }
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_RenderProxy_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_RenderProxy_Current& InCurrent) const
        -> void
    {
        if (ck::Is_NOT_Valid(InCurrent.Get_Data()))
        { return; }

        auto& Data = *InCurrent._Data;

        // Remove from manager's tracking
        if (const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
            ck::IsValid(World))
        {
            if (const auto& Subsystem = World->GetSubsystem<UCk_RenderProxyManager_Subsystem>();
                ck::IsValid(Subsystem))
            {
                if (const auto& ManagerComponent = Subsystem->Get_ManagerComponent();
                    ck::IsValid(ManagerComponent))
                {
                    ManagerComponent->ProxyToEntityMap.Remove(InCurrent.Get_InstanceId());
                }
            }
        }

        // Remove from scene
        if (NOT InHandle.Has<FTag_RenderProxy_Disabled>())
        {
            if (ck::IsValid(Data.SceneDesc.World) && ck::IsValid(Data.SceneDesc.World->Scene, ck::IsValid_Policy_NullptrOnly{}))
            {
                Data.SceneDesc.World->Scene->RemovePrimitive(&Data.SceneDesc);
            }
        }

        // Enqueue proxy deletion on render thread
        auto ProxyToDelete = InCurrent._Proxy;
        ENQUEUE_RENDER_COMMAND(FRenderProxy_Destroy)(
            [ProxyToDelete, DataToDelete = std::move(InCurrent._Data)](FRHICommandListImmediate& RHICmdList) mutable
            {
                // Proxy will be deleted by render thread
                DataToDelete.Reset();
            }
        );

        InCurrent._Proxy = nullptr;

        render_proxy::Verbose(TEXT("Destroyed RenderProxy [{}]"), InHandle);
    }
}

// --------------------------------------------------------------------------------------------------------------------