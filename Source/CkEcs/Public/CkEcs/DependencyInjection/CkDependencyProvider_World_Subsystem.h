#pragma once

#include "CkDependencyProvider_Common.h"
#include "CkDependencyProvider_Request.h"

#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkDependencyProvider_World_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_EntityScript_UE;

// --------------------------------------------------------------------------------------------------------------------

// World-scoped dependency-provider container. One per UWorld. Dies with the
// world — no explicit unregister required on normal teardown.
//
// Inherits UCk_Game_WorldSubsystem_Base_UE so the subsystem is only created
// for in-game worlds (PIE / runtime), not for editor inert worlds, preview
// worlds, asset-edit worlds, etc. Those don't host gameplay providers, so
// instantiating a provider map there is wasted.
//
// Storage shape:
//   _Providers     : direct handle entries (Register call)
//   _Factories     : on-demand handle producers (RegisterFactory)
//   _PendingByType : pending callbacks queued when a Resolve missed; flushed
//                    when a matching Register arrives
//
// Re-entrancy: DoFirePendingFor drains its bucket via FindAndRemoveChecked
// before iterating, so any callback that itself calls Register for a
// different type during the loop does not invalidate the iterator.
UCLASS()
class CKECS_API UCk_DependencyProvider_World_Subsystem_UE : public UCk_Game_WorldSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    using FFactory = TFunction<FCk_Handle(const FCk_Handle& InRequester)>;

    struct FPendingResolution
    {
        TWeakObjectPtr<UCk_EntityScript_UE> _Script;
        FCk_Handle                           _Entity;
        // Fired when the matching provider lands. Implementor writes the
        // resolved handle into the EntityScript's property and may transition
        // the entity out of the awaiting-dependencies state.
        TFunction<void(const FCk_Handle& InResolved)> _OnResolved;
    };

public:
    auto Register(const FCk_Request_DependencyProvider_Register& InRequest) -> void;
    auto Unregister(UScriptStruct* InHandleType) -> void;
    auto Resolve(UScriptStruct* InHandleType) const -> FCk_Handle;

    auto RegisterFactory(UScriptStruct* InHandleType, FFactory InFactory) -> void;

    auto RegisterPending(UScriptStruct* InHandleType, FPendingResolution InPending) -> void;

    // Called on EntityScript teardown to drop any pending entries the script
    // had queued. Prevents pending-map leaks for entities that die before
    // their dependencies resolve.
    auto UnregisterPending(UCk_EntityScript_UE* InScript) -> void;

    // Diagnostic helpers — used by the Phase 0 spike and Phase 2 validator.
    auto Get_RegisteredTypes() const -> TArray<UScriptStruct*>;
    auto Get_PendingCount_ForType(UScriptStruct* InHandleType) const -> int32;

    // UCk_Game_WorldSubsystem_Base_UE
    virtual auto Deinitialize() -> void override;

private:
    // Raw UScriptStruct* keys: UScriptStruct CDOs are engine-lifetime
    // statics, no GC concerns. Skipping TObjectPtr/UPROPERTY here keeps the
    // map key type consistent with the sibling _Factories / _PendingByType
    // maps (which hold non-reflected TFunctions and so can't be UPROPERTY
    // anyway). All three maps are cleared in Deinitialize.
    TMap<UScriptStruct*, FCk_Handle>                            _Providers;
    TMap<UScriptStruct*, FFactory>                              _Factories;
    TMap<UScriptStruct*, TArray<FPendingResolution>>            _PendingByType;

    auto DoFirePendingFor(UScriptStruct* InHandleType, const FCk_Handle& InResolved) -> void;
};
