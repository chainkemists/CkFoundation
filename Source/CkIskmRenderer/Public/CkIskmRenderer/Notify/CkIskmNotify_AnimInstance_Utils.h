#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "CkIskmRenderer/Proxy/CkIskmProxy_Fragment_Data.h"

#include "CkIskmNotify_AnimInstance_Utils.generated.h"

class UCk_IskmNotify_AnimInstance;

// Accessors for the owning proxy/entity carried on a UCk_IskmNotify_AnimInstance.
// The AnimInstance stores its _OwningHandle privately; these utils expose it per
// the utils-as-public-API standard (the function takes the AnimInstance), so anim
// code (e.g. a master AnimInstance running on a proxy-rendered character) can reach
// back to the controlled ECS entity — the proxy's SKMC is owned by the shared
// renderer actor, so TryGetPawnOwner() is null and the entity is the only link.
UCLASS(NotBlueprintable)
class CKISKMRENDERER_API UCk_Utils_IskmNotify_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_IskmNotify_UE);

public:
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmNotify",
        DisplayName="[Ck][IskmNotify] Get Owning Proxy Handle")
    static FCk_Handle_IskmProxy
    Get_OwningProxyHandle(
        const UCk_IskmNotify_AnimInstance* InAnimInstance);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmNotify",
        DisplayName="[Ck][IskmNotify] Get Owning Entity")
    static FCk_Handle
    Get_OwningEntity(
        const UCk_IskmNotify_AnimInstance* InAnimInstance);
};
