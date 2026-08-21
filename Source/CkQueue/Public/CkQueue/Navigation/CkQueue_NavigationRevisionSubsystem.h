#pragma once

#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include "CkQueue_NavigationRevisionSubsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class ANavigationData;

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKQUEUE_API UCk_Queue_NavigationRevisionSubsystem_UE : public UCk_Game_WorldSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Queue_NavigationRevisionSubsystem_UE);

public:
    auto PostInitialize() -> void override;
    auto Deinitialize() -> void override;
    auto TryEnsureBound() -> bool;

private:
    UFUNCTION()
    void OnNavigationGenerationFinished(ANavigationData* InNavigationData);

private:
    int32 _Revision = 0;
    bool _IsBound = false;

public:
    CK_PROPERTY_GET(_Revision);
};

// --------------------------------------------------------------------------------------------------------------------
