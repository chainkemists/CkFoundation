#pragma once

#include "CkEcs/Handle/CkHandle.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkRequest_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKECS_API FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Base);

private:
    mutable FCk_Handle _RequestHandle;

public:
    auto
    PopulateRequestHandle(
        FCk_Handle InOwner) const -> FCk_Handle;

    auto
    GetAndDestroyRequestHandle() const -> FCk_Handle;

    auto
    Get_IsRequestHandleValid() const -> bool;

public:
    // CK_PROPERTY_GET(_RequestHandle);
};

// --------------------------------------------------------------------------------------------------------------------
