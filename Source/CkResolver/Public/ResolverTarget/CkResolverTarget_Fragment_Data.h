#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "ResolverDataBundle/CkResolverDataBundle_Fragment_Data.h"

#include "CkResolverTarget_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRESOLVER_API FCk_Fragment_ResolverTarget_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_ResolverTarget_ParamsData);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRESOLVER_API FCk_Request_ResolverTarget_InitiateNewResolution : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_ResolverTarget_InitiateNewResolution);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FCk_Handle_ResolverDataBundle _DataBundle;

public:
    CK_PROPERTY_GET(_DataBundle);

    CK_DEFINE_CONSTRUCTORS(FCk_Request_ResolverTarget_InitiateNewResolution, _DataBundle);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_ResolverTarget_OnNewResolverDataBundle,
    FCk_Handle_ResolverTarget, InTarget,
    FCk_Handle_ResolverDataBundle, InDataBundle);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_ResolverTarget_OnNewResolverDataBundle_MC,
    FCk_Handle_ResolverTarget, InTarget,
    FCk_Handle_ResolverDataBundle, InDataBundle);

// --------------------------------------------------------------------------------------------------------------------
