#pragma once

#include "CkEcs/Handle/CkHandle.h"

#include "CkCore/Macros/CkMacros.h"

#include "HAL/UnrealMemory.h"
#include "Templates/UnrealTemplate.h"

#include "CkRequest_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Base for all C++-only requests. One-shot: PopulateRequestHandle may be called at most once
    // per struct (a second call returns an invalid handle) — construct a fresh request per submission.
    struct CKECS_API FRequest_Base
    {
    public:
        CK_GENERATED_BODY(FRequest_Base);

    private:
        mutable FCk_Handle _RequestHandle;

    public:
        auto
            PopulateRequestHandle(
                const FCk_Handle& InOwner) const
                -> FCk_Handle;

        auto
            GetAndDestroyRequestHandle() const
                -> FCk_Handle;

        auto
            Get_IsRequestHandleValid() const
                -> bool;

        auto
            Request_TransferHandleToOther(
                const FRequest_Base& InOther) const
                -> void;

#if NOT CK_DISABLE_ECS_HANDLE_DEBUGGING
protected:
        virtual auto
            Get_RequestDebugName() const
                -> FName;

public:
         virtual ~FRequest_Base() = default;
#endif
    };
}

// --------------------------------------------------------------------------------------------------------------------

// Base for all requests that may need to be BP exposed. One-shot: PopulateRequestHandle may be
// called at most once per struct — construct a fresh request per submission.
USTRUCT(BlueprintType)
struct CKECS_API FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Base);

public:
    auto
        PopulateRequestHandle(
            const FCk_Handle& InOwner) const
            -> FCk_Handle;

    auto
        GetAndDestroyRequestHandle() const
            -> FCk_Handle;

    auto
        Get_IsRequestHandleValid() const
            -> bool;

    auto
        Request_TransferHandleToOther(
            const ck::FRequest_Base& InOther) const
            -> void;

    auto
        Request_TransferHandleToOther(
            const ThisType& InOther) const
            -> void;

#if NOT CK_DISABLE_ECS_HANDLE_DEBUGGING
protected:
    virtual auto
        Get_RequestDebugName() const
            -> FName;

public:
    virtual ~FCk_Request_Base() = default;
#endif

private:
    ck::FRequest_Base _RequestBase;
};

// --------------------------------------------------------------------------------------------------------------------

#if CK_DISABLE_ECS_HANDLE_DEBUGGING

#define CK_REQUEST_DEFINE_DEBUG_NAME(_Name_)

#else

#define CK_REQUEST_DEFINE_DEBUG_NAME(_Name_)\
protected:\
    auto Get_RequestDebugName() const -> FName final { return #_Name_; }

#endif

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Fires TSignal at scope exit if the request still owns a valid handle, building the payload
    // then so it can read locals the handler mutated (typically the result enum).
    // Captures-by-reference invariant: every reference the payload-builder closes over MUST outlive
    // this guard — declare the guard AFTER the locals it captures and do not reorder them.
    template <typename TRequest, typename TSignal, typename TPayloadBuilder>
    struct TRequestResultGuard
    {
        TRequestResultGuard(const TRequest& InRequest, TPayloadBuilder InBuildPayload)
            : _Request{InRequest}, _BuildPayload{MoveTemp(InBuildPayload)} {}

        ~TRequestResultGuard()
        {
            if (_Request.Get_IsRequestHandleValid())
            {
                TSignal::Broadcast(_Request.GetAndDestroyRequestHandle(), _BuildPayload());
            }
        }

        TRequestResultGuard(const TRequestResultGuard&) = delete;
        TRequestResultGuard& operator=(const TRequestResultGuard&) = delete;
        TRequestResultGuard(TRequestResultGuard&&) = delete;
        TRequestResultGuard& operator=(TRequestResultGuard&&) = delete;

    private:
        const TRequest& _Request;
        TPayloadBuilder _BuildPayload;
    };

    // TSignal leads so call sites read `ck::MakeRequestResultGuard<TSignal>(InRequest, [&]{ ... })`
    // and the remaining types deduce.
    template <typename TSignal, typename TRequest, typename TPayloadBuilder>
    auto MakeRequestResultGuard(const TRequest& InRequest, TPayloadBuilder InBuildPayload)
    {
        return TRequestResultGuard<TRequest, TSignal, TPayloadBuilder>{
            InRequest, MoveTemp(InBuildPayload)};
    }
}

// --------------------------------------------------------------------------------------------------------------------
