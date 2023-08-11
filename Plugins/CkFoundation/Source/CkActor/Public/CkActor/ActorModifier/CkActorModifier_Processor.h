#pragma once

#include "CkActorModifier_Fragment.h"
#include "CkActorModifier_Fragment_Params.h"

#include "CkActor/ActorInfo/CkActorInfo_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKACTOR_API FCk_Processor_ActorModifier_Location_HandleRequests
        : public TProcessor<FCk_Processor_ActorModifier_Location_HandleRequests, FCk_Fragment_ActorInfo_Current, FCk_Fragment_ActorModifier_LocationRequests>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(const FTimeType& InDeltaT,
            FHandleType InHandle,
            const FCk_Fragment_ActorInfo_Current& InActorInfoComp,
            FCk_Fragment_ActorModifier_LocationRequests& InRequestsComp) const -> void;

    private:
        auto DoHandleRequest(FHandleType InHandle,
            AActor* InActor,
            const FCk_Request_ActorModifier_SetLocation& InRequest) const -> void;

        auto DoHandleRequest(FHandleType InHandle,
            AActor* InActor,
            const FCk_Request_ActorModifier_AddLocationOffset& InRequest) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKACTOR_API FCk_Processor_ActorModifier_Scale_HandleRequests
        : public TProcessor<FCk_Processor_ActorModifier_Scale_HandleRequests, FCk_Fragment_ActorInfo_Current, FCk_Fragment_ActorModifier_ScaleRequests>
    {
    public:
        using MarkedDirtyBy = FCk_Fragment_ActorModifier_ScaleRequests;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(const FTimeType& InDeltaT,
            FHandleType InHandle,
            const FCk_Fragment_ActorInfo_Current& InActorInfoComp,
            FCk_Fragment_ActorModifier_ScaleRequests& InRequestsComp) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKACTOR_API FCk_Processor_ActorModifier_Rotation_HandleRequests
        : public TProcessor<FCk_Processor_ActorModifier_Rotation_HandleRequests, FCk_Fragment_ActorInfo_Current, FCk_Fragment_ActorModifier_RotationRequests>
    {
    public:
        using MarkedDirtyBy = FCk_Fragment_ActorModifier_RotationRequests;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(const FTimeType& InDeltaT,
            FHandleType InHandle,
            const FCk_Fragment_ActorInfo_Current& InActorInfoComp,
            FCk_Fragment_ActorModifier_RotationRequests& InRequestsComp) const -> void;

    private:
        auto DoHandleRequest(FHandleType InHandle,
            AActor* InActor,
            const FCk_Request_ActorModifier_SetRotation& InRequest) const -> void;

        auto DoHandleRequest(FHandleType InHandle,
            AActor* InActor,
            const FCk_Request_ActorModifier_AddRotationOffset& InRequest) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKACTOR_API FCk_Processor_ActorModifier_Transform_HandleRequests
        : public TProcessor<FCk_Processor_ActorModifier_Transform_HandleRequests, FCk_Fragment_ActorInfo_Current, FCk_Fragment_ActorModifier_TransformRequests>
    {
    public:
        using MarkedDirtyBy = FCk_Fragment_ActorModifier_TransformRequests;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(const FTimeType& InDeltaT,
            FHandleType InHandle,
            const FCk_Fragment_ActorInfo_Current& InActorInfoComp,
            FCk_Fragment_ActorModifier_TransformRequests& InRequestsComp) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKACTOR_API FCk_Processor_ActorModifier_SpawnActor_HandleRequests
        : public TProcessor<FCk_Processor_ActorModifier_SpawnActor_HandleRequests, FCk_Fragment_ActorModifier_SpawnActorRequests>
    {
    public:
        using MarkedDirtyBy = FCk_Fragment_ActorModifier_SpawnActorRequests;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(const FTimeType& InDeltaT,
            FHandleType InHandle,
            FCk_Fragment_ActorModifier_SpawnActorRequests& InRequests) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKACTOR_API FCk_Processor_ActorModifier_AddActorComponent_HandleRequests
        : public TProcessor<FCk_Processor_ActorModifier_AddActorComponent_HandleRequests, FCk_Fragment_ActorInfo_Current, FCk_Fragment_ActorModifier_AddActorComponentRequests>
    {
    public:
        using MarkedDirtyBy = FCk_Fragment_ActorModifier_AddActorComponentRequests;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            const FTimeType& InDeltaT,
            FHandleType InHandle,
            const FCk_Fragment_ActorInfo_Current& InActorInfoComp,
            FCk_Fragment_ActorModifier_AddActorComponentRequests& InRequests) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------
