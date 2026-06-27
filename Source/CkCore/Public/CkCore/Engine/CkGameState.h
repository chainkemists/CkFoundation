#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <GameFramework/GameStateBase.h>

#include "CkGameState.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable)
class CKCORE_API ACk_GameState_UE : public AGameStateBase
{
    GENERATED_BODY()
public:
    CK_GENERATED_BODY(ACk_GameState_UE);

public:
    ACk_GameState_UE();

public:
    auto Tick(
        float InDeltaSeconds) -> void override;

public:
    auto GetLifetimeReplicatedProps(
            TArray<FLifetimeProperty>&) const -> void override;

public:
    auto PostActorCreated() -> void override;
protected:
    auto BeginPlay() -> void override;

protected:
    UFUNCTION(BlueprintImplementableEvent)
    void
    OnPlayerAdded(
        APlayerState* InPlayerState);

    UFUNCTION(BlueprintImplementableEvent)
    void
    OnPlayerRemoved(
        APlayerState* InPlayerState);

public:
    UFUNCTION(BlueprintCallable)
    float
    Get_ServerFPS() const;

    // The server's build id (short git hash), replicated to all clients so each client can compare it
    // against its own and flag a version mismatch. Empty until the server's BeginPlay has set it.
    UFUNCTION(BlueprintCallable)
    FString
    Get_ServerBuildId() const;

private:
    UPROPERTY(Replicated)
    float _ServerFPS;

    UPROPERTY(Replicated)
    FString _ServerBuildId;

protected:
    auto
    AddPlayerState(
        APlayerState* InPlayerState) -> void override;

    auto
    RemovePlayerState(APlayerState* InPlayerState) -> void override;
};

// --------------------------------------------------------------------------------------------------------------------
