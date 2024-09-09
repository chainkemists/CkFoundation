#pragma once

#include "CkAntSquad_Fragment.h"
#include "CkOwningActor_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

class UAntSubsystem;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKANTBRIDGE_API FProcessor_AntSquad_HandleRequests : public ck_exp::TProcessor<
            FProcessor_AntSquad_HandleRequests,
            FCk_Handle_AntSquad,
            FFragment_AntSquad_Current,
            FFragment_AntSquad_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_AntSquad_Requests;

    public:
        FProcessor_AntSquad_HandleRequests(
            const RegistryType& InRegistry,
            const TWeakObjectPtr<UWorld> InWorld);


    public:
        auto DoTick(
            TimeType InDeltaT) -> void;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_AntSquad_Current& InCurrent,
            FFragment_AntSquad_Requests& InRequestsComp) const -> void;

    private:
        auto
        DoHandleRequest(
            HandleType& InHandle,
            FFragment_AntSquad_Current& InCurrent,
            const FCk_Request_AntSquad_AddAgent& InRequestAgent) const -> void;

    public:
        TWeakObjectPtr<UAntSubsystem> _AntSubsystem;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKANTBRIDGE_API FProcessor_AntAgent_Renderer_Setup : public ck_exp::TProcessor<
        FProcessor_AntAgent_Renderer_Setup,
        FCk_Handle_AntAgent_Renderer,
        FFragment_AntAgent_Renderer_Params,
        FFragment_OwningActor_Current,
        FTag_AntAgent_Renderer_NeedsSetup,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_AntAgent_Renderer_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AntAgent_Renderer_Params& InParams,
            const FFragment_OwningActor_Current& InOwningActorCurrent) -> void;

    private:
        TMap<TWeakObjectPtr<UStaticMesh>, int32> _MeshToType;
        int32 _NextAvailableMeshType = -1;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKANTBRIDGE_API FProcessor_AntAgent_Renderer_ClearInstances : public ck_exp::TProcessor<
        FProcessor_AntAgent_Renderer_ClearInstances,
        FCk_Handle_AntAgent_Renderer,
        FFragment_AntAgent_Renderer_Current,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AntAgent_Renderer_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKANTBRIDGE_API FProcessor_InstancedStaticMeshRenderer_HandleRequests : public ck_exp::TProcessor<
        FProcessor_InstancedStaticMeshRenderer_HandleRequests,
        FCk_Handle_AntAgent_Renderer,
        FFragment_AntAgent_Renderer_Current,
        FFragment_InstancedStaticMeshRenderer_Requests,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_InstancedStaticMeshRenderer_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto DoTick(
            TimeType InDeltaT) -> void;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AntAgent_Renderer_Current& InCurrent,
            FFragment_InstancedStaticMeshRenderer_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_AntAgent_Renderer_Current& InCurrent,
            const FCk_Request_InstancedStaticMeshRenderer_NewInstance& InRequest)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKANTBRIDGE_API FProcessor_AntAgent_Renderer_Update
    {
    public:
        using RegistryType = FCk_Registry;
        using TimeType = FCk_Time;

    public:
        FProcessor_AntAgent_Renderer_Update(
            const RegistryType& InRegistry,
            const TWeakObjectPtr<UWorld> InWorld);

    public:
        auto
        Tick(
            TimeType InDeltaT) -> void;

    private:
        RegistryType _Registry;
        TWeakObjectPtr<UAntSubsystem> _AntSubsystem;
    };
}

// --------------------------------------------------------------------------------------------------------------------
