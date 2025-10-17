#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <GameFramework/Actor.h>
#include <Components/SceneComponent.h>
#include <Components/PrimitiveComponent.h>
#include <Components\ComponentInterfaces.h>

#include "CkRenderProxyManager_Actor.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintType, NotBlueprintable)
class CKRENDERPROXY_API UCk_Component_RenderProxyManager : public UPrimitiveComponent/*, public IPrimitiveComponent*/
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Component_RenderProxyManager);

public:
    UCk_Component_RenderProxyManager();

public:
    //// IPrimitiveComponent interface - required for hit proxies
    //auto GetUObject() const -> UObject* override;

    // FPrimitiveSceneProxy creation - not needed for our use case
    auto CreateSceneProxy() -> FPrimitiveSceneProxy* override { return nullptr; }

public:
    // Mapping from InstanceId to Entity Handle for editor selection
    UPROPERTY()
    TMap<FGuid, FCk_Handle> ProxyToEntityMap;

    // Currently selected proxy in editor (for details panel)
    UPROPERTY()
    FGuid SelectedProxyInstanceId;

    UPROPERTY()
    FCk_Handle SelectedProxyEntity;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintType, NotBlueprintable)
class CKRENDERPROXY_API ACk_RenderProxyManager_Actor : public AActor
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ACk_RenderProxyManager_Actor);

public:
    ACk_RenderProxyManager_Actor();

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta=(AllowPrivateAccess))
    TObjectPtr<USceneComponent> _RootNode;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta=(AllowPrivateAccess))
    TObjectPtr<UCk_Component_RenderProxyManager> _ProxyManagerComponent;

public:
    CK_PROPERTY_GET(_ProxyManagerComponent);
};

// --------------------------------------------------------------------------------------------------------------------