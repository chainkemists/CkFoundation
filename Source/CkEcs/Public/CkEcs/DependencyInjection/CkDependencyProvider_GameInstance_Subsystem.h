#pragma once

#include "CkDependencyProvider_Common.h"
#include "CkDependencyProvider_Request.h"

#include "CkEcs/Handle/CkHandle.h"

#include "Subsystems/GameInstanceSubsystem.h"

#include "CkDependencyProvider_GameInstance_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_EntityScript_UE;

// --------------------------------------------------------------------------------------------------------------------

// GameInstance-scoped dependency-provider container. One per UGameInstance —
// survives map travel. Identical API shape to the World variant; storage
// lives on the GameInstance subsystem so it can't accidentally be wiped by a
// world swap.
//
// Use this scope only for providers that conceptually outlive a single map.
// Most game-mode-driven systems should be World-scoped.
UCLASS()
class CKECS_API UCk_DependencyProvider_GameInstance_Subsystem_UE : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    using FFactory = TFunction<FCk_Handle(const FCk_Handle& InRequester)>;

    struct FPendingResolution
    {
        TWeakObjectPtr<UCk_EntityScript_UE> _Script;
        FCk_Handle                           _Entity;
        TFunction<void(const FCk_Handle& InResolved)> _OnResolved;
    };

public:
    auto Register(const FCk_Request_DependencyProvider_Register& InRequest) -> void;
    auto Unregister(UScriptStruct* InHandleType) -> void;
    auto Resolve(UScriptStruct* InHandleType) const -> FCk_Handle;

    auto RegisterFactory(UScriptStruct* InHandleType, FFactory InFactory) -> void;

    auto RegisterPending(UScriptStruct* InHandleType, FPendingResolution InPending) -> void;
    auto UnregisterPending(UCk_EntityScript_UE* InScript) -> void;

    auto Get_RegisteredTypes() const -> TArray<UScriptStruct*>;
    auto Get_PendingCount_ForType(UScriptStruct* InHandleType) const -> int32;

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
