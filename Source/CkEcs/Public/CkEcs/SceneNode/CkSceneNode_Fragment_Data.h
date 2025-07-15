#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include <GameplayTagContainer.h>

#include "CkSceneNode_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_SceneNode_HandleRequests;
}

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKECS_API FCk_Handle_SceneNode : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_SceneNode); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_SceneNode);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKECS_API FCk_Request_SceneNode_UpdateRelativeTransform : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_SceneNode_UpdateRelativeTransform);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_SceneNode_UpdateRelativeTransform);

public:
    friend class ck::FProcessor_SceneNode_HandleRequests;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    FTransform _NewRelativeTransform;

public:
    CK_PROPERTY_GET(_NewRelativeTransform);

    CK_DEFINE_CONSTRUCTORS(FCk_Request_SceneNode_UpdateRelativeTransform, _NewRelativeTransform);
};

// --------------------------------------------------------------------------------------------------------------------