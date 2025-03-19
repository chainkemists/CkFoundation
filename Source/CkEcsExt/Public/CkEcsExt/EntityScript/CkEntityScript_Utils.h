#pragma once

#include "CkEcs/Handle/CkHandle.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcsExt/EntityScript/CkEntityScript_Fragment_Data.h"

#include "CkEntityScript_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKECSEXT_API UCk_Utils_EntityScript_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_EntityScript_UE);

public:
    UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
    static void
    Request_SpawnEntity(
        FCk_Handle InLifetimeOwner,
        UCk_EntityScript_PDA* InEntityScript);
};

// --------------------------------------------------------------------------------------------------------------------
