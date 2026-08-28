#pragma once

#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include "CkNavigationRevision_Subsystem.generated.h"

class ANavigationData;

// World-scoped monotonic signal for completed navigation generations. Consumers snapshot the
// revision before dirtying nav data and wait for it to advance instead of guessing rebuild latency.
UCLASS()
class CKNAVIGATION_API UCk_NavigationRevisionSubsystem_UE : public UCk_Game_WorldSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_NavigationRevisionSubsystem_UE);

    auto PostInitialize() -> void override;
    auto Deinitialize() -> void override;
    auto TryEnsureBound() -> bool;

private:
    UFUNCTION()
    void OnNavigationGenerationFinished(ANavigationData* InNavigationData);

private:
    uint64 _Revision = 0;
    bool _IsBound = false;

public:
    CK_PROPERTY_GET(_Revision);
};

// --------------------------------------------------------------------------------------------------------------------
