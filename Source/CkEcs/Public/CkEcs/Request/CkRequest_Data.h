#pragma once

#include "CkEcs/Handle/CkHandle.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkRequest_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Should be used as the base for all c++ only requests
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

// Should be used as the base for all requests that may need to be BP exposed
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
