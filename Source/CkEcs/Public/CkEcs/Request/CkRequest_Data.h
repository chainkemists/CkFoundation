#pragma once

#include "CkEcs/Handle/CkHandle.h"

#include "CkCore/Macros/CkMacros.h"

#include "HAL/UnrealMemory.h"
#include "Templates/UnrealTemplate.h"

#include "CkRequest_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Should be used as the base for all c++ only requests.
    // One-shot: PopulateRequestHandle may be called at most once per struct (a second call returns an
    // invalid handle), so construct a fresh request per submission rather than reusing one.
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

// Should be used as the base for all requests that may need to be BP exposed.
// One-shot: PopulateRequestHandle may be called at most once per struct (a second call returns an
// invalid handle), so construct a fresh request per submission rather than reusing one.
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
    // Fires a per-request completion signal at scope exit if the request still owns a valid request
    // handle. The payload is rebuilt from the supplied lambda at destruction so it can capture
    // by-reference any locals the handler mutates (typically the result enum).
    //
    // Captures-by-reference invariant: any reference the payload-builder closes over MUST outlive
    // this guard. Declare the guard *after* the locals it captures and do not reorder them.
    //
    // TRequest must satisfy the FRequest_Base shape (`Get_IsRequestHandleValid`,
    // `GetAndDestroyRequestHandle`); TSignal must expose a static `Broadcast(handle, payload)`.
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

    // Factory: keeps TSignal as the leading explicit template parameter so call sites read
    // `ck::MakeRequestResultGuard<TSignal>(InRequest, [&]{ ... })` — the signal is the part the
    // caller cares about; the request and payload-builder types deduce.
    template <typename TSignal, typename TRequest, typename TPayloadBuilder>
    auto MakeRequestResultGuard(const TRequest& InRequest, TPayloadBuilder InBuildPayload)
    {
        return TRequestResultGuard<TRequest, TSignal, TPayloadBuilder>{
            InRequest, MoveTemp(InBuildPayload)};
    }
}

// --------------------------------------------------------------------------------------------------------------------
