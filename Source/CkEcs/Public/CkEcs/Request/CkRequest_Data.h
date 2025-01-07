#pragma once

#include "CkEcs/Handle/CkHandle.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkRequest_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
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
    };
}

// --------------------------------------------------------------------------------------------------------------------

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

private:
    ck::FRequest_Base _RequestBase;
};

// --------------------------------------------------------------------------------------------------------------------
