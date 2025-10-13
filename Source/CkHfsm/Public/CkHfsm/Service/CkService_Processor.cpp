#include "CkService_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkHFSM/CkHFSM_Log.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Service_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Service_Current& InCurrent) const
        -> void
    {
        InHandle.Remove<FTag_Service_Setup>();

        hfsm::VeryVerbose(TEXT("[SETUP][SERVICE] Entity [{}]"), InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Service_Enter::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Service_Current& InCurrent) const
        -> void
    {
        InHandle.Remove<FTag_Service_Enter>();
        InHandle.AddOrGet<FTag_Service_Update>();

        {
#if STATS
            auto StatCounter = FScopeCycleCounter{InHandle.Get<TStatId>()};
#endif
            UUtils_Signal_OnServiceStart::Broadcast(InHandle, MakePayload(InHandle, InDeltaT));
        }

        hfsm::VeryVerbose(TEXT("[ENTER][SERVICE] Entity [{}]"), InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Service_Exit::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Service_Current& InCurrent) const
        -> void
    {
        InHandle.Remove<FTag_Service_Exit>();
        InHandle.Remove<FTag_Service_Update>();

        {
#if STATS
            auto StatCounter = FScopeCycleCounter{InHandle.Get<TStatId>()};
#endif
            UUtils_Signal_OnServiceStop::Broadcast(InHandle, MakePayload(InHandle, InDeltaT));
        }

        hfsm::VeryVerbose(TEXT("[EXIT][SERVICE] Entity [{}]"), InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Service_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Service_Current& InCurrent) const
        -> void
    {
        // Derived service implementations will extend this behavior
        // Base service just stays alive
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Service_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Service_Current& InCurrent,
            const FFragment_Service_Requests& InRequests) const
        -> void
    {
        InHandle.CopyAndRemove(InRequests, [&](FFragment_Service_Requests& InRequestsCopy)
        {
            algo::ForEachRequest(InRequestsCopy._Requests, Visitor([&](const auto& InRequest)
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
        FProcessor_Service_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Service_Current& InCurrent,
            const FCk_Request_Service_Command& InRequest)
        -> void
    {
        switch (InRequest.Get_Command())
        {
            case ECk_Service_Command::Start:
            {
                InHandle.AddOrGet<FTag_Service_Enter>();
                break;
            }
            case ECk_Service_Command::Stop:
            {
                InHandle.AddOrGet<FTag_Service_Exit>();
                break;
            }
            default:
            {
                CK_INVALID_ENUM(InRequest.Get_Command());
                break;
            }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------