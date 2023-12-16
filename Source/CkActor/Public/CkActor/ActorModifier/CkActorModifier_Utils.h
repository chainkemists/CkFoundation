#pragma once
#include "CkActorModifier_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkActorModifier_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKACTOR_API UCk_Utils_ActorModifier_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ActorModifier_UE);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ActorModifier",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static void
    Request_SpawnActor(
        FCk_Handle InHandle,
        const FCk_Request_ActorModifier_SpawnActor& InRequest,
        const FCk_Delegate_ActorModifier_OnActorSpawned& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ActorModifier",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static void
    Request_AddActorComponent(
        FCk_Handle InHandle,
        const FCk_Request_ActorModifier_AddActorComponent& InRequest,
        const FCk_Delegate_ActorModifier_OnActorComponentAdded& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
