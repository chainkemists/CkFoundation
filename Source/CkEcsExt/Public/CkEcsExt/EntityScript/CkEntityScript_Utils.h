#pragma once

#include "CkEcs/Handle/CkHandle.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcsExt/EntityScript/CkEntityScript_Fragment_Data.h"

#include "CkEntityScript_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_EntityScript_SpawnEntity_HandleRequests;
    class FProcessor_EntityScript_BeginPlay;
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKECSEXT_API UCk_Utils_EntityScript_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_EntityScript_UE);

public:
    friend class ck::FProcessor_EntityScript_SpawnEntity_HandleRequests;
    friend class ck::FProcessor_EntityScript_BeginPlay;

public:
    UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
    static void
    Request_SpawnEntity(
        UPARAM(ref) FCk_Handle& InLifetimeOwner,
        UCk_EntityScript_UE* InEntityScript);

private:
    static auto
    DoAdd(
        FCk_Handle& InHandle,
        UCk_EntityScript_UE* InEntityScript) -> void;

    static auto
    Request_MarkEntityScript_AsHasBegunPlay(
        FCk_Handle& InHandle) -> void;

};
// --------------------------------------------------------------------------------------------------------------------
