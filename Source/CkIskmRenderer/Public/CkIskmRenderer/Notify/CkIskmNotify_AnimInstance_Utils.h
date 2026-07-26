#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "CkIskmRenderer/Proxy/CkIskmProxy_Fragment_Data.h"

#include "CkIskmNotify_AnimInstance_Utils.generated.h"

class UCk_IskmNotify_AnimInstance;

// Anim code reaches its controlled ECS entity through here: the proxy's SKMC is owned by the shared
// renderer actor, so TryGetPawnOwner() is null and the owning handle is the only link back.
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
};
