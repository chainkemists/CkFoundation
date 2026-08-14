#include "CkInputSource_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkInput/CkInput_Log.h"
#include "CkInput/CkInputSource_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_GROUP(ck::FGroup_Input_Collect);
CK_REGISTER_GROUP(ck::FGroup_Input_Route);

CK_REGISTER_PROCESSOR(ck::FProcessor_InputSource_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_InputSource_CancelPendingRequests);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_InputSource_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InInputSource,
            FFragment_InputSource_Current& InCurrent,
            FFragment_InputSource_Requests& InRequests) const
        -> void
    {
        const auto RequestsCopy = InRequests._Requests;
        InRequests._Requests.Reset();

        algo::ForEachRequest(RequestsCopy, ck::Visitor(
        [&](const auto& InRequest) -> void
        {
            auto Result = ECk_Request_OperationResult::Failed;
            const auto Guard = MakeCompletionGuard(InRequest, InInputSource, Result);

            Result = DoHandleRequest(InInputSource, InCurrent, InRequest);
        }), policy::DontResetContainer{});

        if (InRequests._Requests.IsEmpty())
        {
            InInputSource.Remove<MarkedDirtyBy>();
        }
    }

    auto
        FProcessor_InputSource_HandleRequests::
        DoHandleRequest(
            HandleType InInputSource,
            FFragment_InputSource_Current& InCurrent,
            const FCk_Request_InputSource_InjectRawEvent& InRequest)
        -> ECk_Request_OperationResult
    {
        const auto& Event = InRequest.Get_Event();

        InCurrent._PendingRawEvents.Emplace(Event);

        input::VeryVerbose
        (
            TEXT("InputSource [{}] received raw event [{}] on device [{}|{}] with ordering [{}]"),
            InInputSource,
            Event.Get_EventType(),
            Event.Get_DeviceClass(),
            Event.Get_RawDeviceUserIndex(),
            Event.Get_OrderingFidelity()
        );

        return ECk_Request_OperationResult::Succeeded;
    }

    auto
        FProcessor_InputSource_HandleRequests::
        DoHandleRequest(
            HandleType InInputSource,
            FFragment_InputSource_Current& InCurrent,
            const FCk_Request_InputSource_AssignDevice& InRequest)
        -> ECk_Request_OperationResult
    {
        const auto& DeviceId = InRequest.Get_DeviceId();

        // Re-claiming a device this source already owns leaves the caller's intent holding, so it is a
        // success rather than a conflict.
        if (InCurrent._OwnedDevices.Contains(DeviceId))
        { return ECk_Request_OperationResult::Succeeded; }

        const auto ExistingOwner = UCk_Utils_InputSource_UE::TryGet_SourceOwningDevice(InInputSource, DeviceId);

        const auto DeviceIsUnowned = ck::Is_NOT_Valid(ExistingOwner);
        CK_ENSURE_IF_NOT(DeviceIsUnowned,
            TEXT("Request_AssignDevice on InputSource [{}] rejected: device [{}|{}] is already owned by "
                 "InputSource [{}] and ownership is exclusive"),
            InInputSource, DeviceId.Get_DeviceClass(), DeviceId.Get_RawDeviceUserIndex(), ExistingOwner)
        { return ECk_Request_OperationResult::Failed; }

        InCurrent._OwnedDevices.Emplace(DeviceId);

        input::Verbose
        (
            TEXT("InputSource [{}] took ownership of device [{}|{}]"),
            InInputSource, DeviceId.Get_DeviceClass(), DeviceId.Get_RawDeviceUserIndex()
        );

        return ECk_Request_OperationResult::Succeeded;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_InputSource_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InInputSource,
            const FFragment_InputSource_Requests& InRequests)
        -> void
    {
        request::FireCancelledForPending(InInputSource, InRequests.Get_Requests());
    }
}

// --------------------------------------------------------------------------------------------------------------------
