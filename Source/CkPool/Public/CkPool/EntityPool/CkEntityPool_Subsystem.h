#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include "CkPool/EntityPool/CkEntityPool_Fragment_Data.h"

#include <GameplayTagContainer.h>
#include <Templates/SubclassOf.h>

#include "CkEntityPool_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_EntityScript_UE;

// --------------------------------------------------------------------------------------------------------------------

// Registry + lazy creation ONLY. All pool state lives on pool entities (see CkEntityPool_Fragment.h); all
// mutation flows through requests drained by the EntityPool processors. Pool entities are top-level
// (transient-owner) entities — they live until world teardown or an explicit Request_DestroyPool
UCLASS()
class CKPOOL_API UCk_EntityPool_Subsystem_UE : public UCk_Game_WorldSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EntityPool_Subsystem_UE);

    friend class UCk_Utils_EntityPool_UE;

public:
    auto Deinitialize() -> void override;

private:
    auto
    DoGetOrCreate_Pool(
        const FCk_Fragment_EntityPool_ParamsData& InParams) -> FCk_Handle_EntityPool;

    auto
    DoTryGet_Pool_ByClass(
        const TSubclassOf<UCk_EntityScript_UE>& InEntityScriptClass) -> FCk_Handle_EntityPool;

    auto
    DoTryGet_Pool_ByName(
        const FGameplayTag& InPoolName) -> FCk_Handle_EntityPool;

    auto
    DoForget_Pool(
        const FCk_Handle_EntityPool& InPool) -> void;

private:
    // The class map holds each class's DEFAULT (unnamed) pool; named pools live only in the name map
    UPROPERTY(Transient)
    TMap<TSubclassOf<UCk_EntityScript_UE>, FCk_Handle_EntityPool> _PoolsByClass;

    UPROPERTY(Transient)
    TMap<FGameplayTag, FCk_Handle_EntityPool> _PoolsByName;
};

// --------------------------------------------------------------------------------------------------------------------
