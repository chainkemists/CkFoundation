#pragma once

#include "CkDependencyProvider_Common.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Request/CkRequest_Data.h"

#include <StructUtils/InstancedStruct.h>

#include "CkDependencyProvider_Request.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Request struct for the Register BPFL call. Carries the typed handle as a
// single boxed FInstancedStruct — the contained UScriptStruct (e.g.
// FCk_Handle_DayCycle) serves as the registry key, and the contained bytes
// carry the FCk_Handle value (typesafe handle subclasses are layout-compatible
// with FCk_Handle).
//
// AS authoring: `FCk_Request_DependencyProvider_Register(FInstancedStruct::Make(_TypedHandle))`.
// The Hazelight AS plugin preserves the typed UScriptStruct identity through
// the boxing, so the same call site works for any FCk_Handle_* subclass — no
// dynamic-handle codegen extension required.
USTRUCT(BlueprintType)
struct CKECS_API FCk_Request_DependencyProvider_Register : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_DependencyProvider_Register);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_DependencyProvider_Register);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FInstancedStruct _ProvidedHandle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_DependencyProvider_OverwritePolicy _OverwritePolicy = ECk_DependencyProvider_OverwritePolicy::EnsureOnDuplicate;

public:
    CK_PROPERTY_GET(_ProvidedHandle);
    CK_PROPERTY(_OverwritePolicy);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_DependencyProvider_Register, _ProvidedHandle);
};
