#pragma once

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"

#include <CoreMinimal.h>
#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkActorComponent_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_ActorComponent_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ActorComponent_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ActorComponent",
              DisplayName = "[Ck] Get Allow Tick On Dedicated Server")
    static bool
    Get_AllowTickOnDedicatedServer(
        const UActorComponent* InActorComponent);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ActorComponent",
              DisplayName = "[Ck] Set Allow Tick On Dedicated Server")
    static void
    Set_AllowTickOnDedicatedServer(
        UActorComponent* InActorComponent,
        bool InServerTickEnabled);
};

// --------------------------------------------------------------------------------------------------------------------
