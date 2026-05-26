#pragma once

#include "CkDependencyProvider_Common.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkDependencyProvider_Request.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Request struct for the Register_World / Register_GameInstance BPFL calls.
// Bundles HandleType + ProvidedHandle + OverwritePolicy per project convention §7.
USTRUCT(BlueprintType)
struct CKECS_API FCk_Request_DependencyProvider_Register : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_DependencyProvider_Register);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_DependencyProvider_Register);

private:
    // The typed handle's UScriptStruct, used as the lookup key. AS authors
    // pass `FCk_Handle_<Feature>::StaticStruct()`.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TObjectPtr<UScriptStruct> _HandleType;

    // The actual handle to hand out on Resolve. May be a typed handle —
    // the FCk_Handle storage holds the underlying entity reference; consumers
    // re-cast to the typed handle via the registered type's Cast utility.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Handle _ProvidedHandle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_DependencyProvider_OverwritePolicy _OverwritePolicy = ECk_DependencyProvider_OverwritePolicy::EnsureOnDuplicate;

public:
    CK_PROPERTY_GET(_HandleType);
    CK_PROPERTY_GET(_ProvidedHandle);
    CK_PROPERTY(_OverwritePolicy);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_DependencyProvider_Register, _HandleType, _ProvidedHandle);
};
