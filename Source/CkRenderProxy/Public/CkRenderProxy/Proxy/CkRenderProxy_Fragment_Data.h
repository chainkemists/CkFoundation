#pragma once

#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include <Engine/StaticMesh.h>

#include "CkRenderProxy_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKRENDERPROXY_API FCk_Handle_RenderProxy : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_RenderProxy); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_RenderProxy);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRENDERPROXY_API FCk_Fragment_RenderProxy_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_RenderProxy_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UStaticMesh> _Mesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Mobility _Mobility = ECk_Mobility::Movable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _StartingState = ECk_EnableDisable::Enable;

    // Future expansion slots (not implemented):
    // TArray<FCk_MeshMaterialOverride> _MaterialOverrides;
    // FCk_RenderProxy_CullingInfo _CullingInfo;
    // FCk_RenderProxy_LightingInfo _LightingInfo;
    // int32 _NumCustomData;

public:
    CK_PROPERTY_GET(_Mesh);
    CK_PROPERTY_GET(_Mobility);
    CK_PROPERTY_GET(_StartingState);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_RenderProxy_ParamsData, _Mesh);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRENDERPROXY_API FCk_Request_RenderProxy_EnableDisable : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_RenderProxy_EnableDisable);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_RenderProxy_EnableDisable);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _EnableDisable = ECk_EnableDisable::Disable;

public:
    CK_PROPERTY_GET(_EnableDisable);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_RenderProxy_EnableDisable, _EnableDisable);
};

// --------------------------------------------------------------------------------------------------------------------