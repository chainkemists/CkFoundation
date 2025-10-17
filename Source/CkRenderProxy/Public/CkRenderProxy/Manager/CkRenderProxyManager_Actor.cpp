#include "CkRenderProxyManager_Actor.h"

// --------------------------------------------------------------------------------------------------------------------

UCk_Component_RenderProxyManager::
    UCk_Component_RenderProxyManager()
{
    PrimaryComponentTick.bCanEverTick = false;
    bHiddenInGame = true;
}

//auto
//    UCk_Component_RenderProxyManager::
//    GetUObject() const
//    -> UObject*
//{
//    // Return the component itself, NOT a UPrimitiveComponent
//    // This ensures the engine creates hit proxies through our custom system
//    //return const_cast<UCk_Component_RenderProxyManager*>(this);
//
//    /*
//     * So you'll need to implement IPrimitiveComponent for your proxy setup, make sure for GetObject on the interface that it doesn't return anything that is a UPrimitiveComponent, return the actor instead
//     */
//    auto ThisAsPrimComp = Cast<UPrimitiveComponent>(this);
//    return ThisAsPrimComp->GetOwner();
//}
//
//auto
//    UCk_Component_RenderProxyManager::
//    GetUObject()
//    -> UObject*
//{
//    // Return the component itself, NOT a UPrimitiveComponent
//    // This ensures the engine creates hit proxies through our custom system
//    //return const_cast<UCk_Component_RenderProxyManager*>(this);
//
//    /*
//     * So you'll need to implement IPrimitiveComponent for your proxy setup, make sure for GetObject on the interface that it doesn't return anything that is a UPrimitiveComponent, return the actor instead
//     */
//    auto ThisAsPrimComp = Cast<UPrimitiveComponent>(this);
//    return ThisAsPrimComp->GetOwner();
//}

// --------------------------------------------------------------------------------------------------------------------

ACk_RenderProxyManager_Actor::
    ACk_RenderProxyManager_Actor()
{
    PrimaryActorTick.bCanEverTick = false;
    bNetLoadOnClient = false;

    _RootNode = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = _RootNode;

    _ProxyManagerComponent = CreateDefaultSubobject<UCk_Component_RenderProxyManager>(TEXT("ProxyManager"));
    _ProxyManagerComponent->SetupAttachment(_RootNode);
}

// --------------------------------------------------------------------------------------------------------------------