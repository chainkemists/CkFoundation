//#pragma once
//
//#include "CkDecal_Fragment.h"
//#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
//
//#include "CkEcs/Processor/CkProcessor.h"
//
//// --------------------------------------------------------------------------------------------------------------------
//
//namespace ck
//{
//    class CKGRAPHICS_API FProcessor_Decal_HandleRequests : public TProcessor<
//            FProcessor_Decal_HandleRequests,
//            FFragment_Decal_Requests,
//            CK_IGNORE_PENDING_KILL>
//    {
//    public:
//        using MarkedDirtyBy = FFragment_Decal_Requests;
//
//    public:
//        using TProcessor::TProcessor;
//
//    public:
//        auto DoTick(
//            TimeType InDeltaT) -> void;
//
//    public:
//        auto ForEachEntity(
//            TimeType InDeltaT,
//            HandleType InHandle,
//            FFragment_Decal_Requests& InRequestsComp) const -> void;
//
//    private:
//        static auto
//        DoHandleRequest(
//            HandleType InHandle,
//            const FCk_Request_Decal_QueryRenderedActors& InRequest) -> void;
//    };
//}
//
//// --------------------------------------------------------------------------------------------------------------------
