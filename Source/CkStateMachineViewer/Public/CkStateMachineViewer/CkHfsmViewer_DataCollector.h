#pragma once

#include "CkHfsmViewer_Types.h"

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

class UWorld;

namespace ck { struct FFragment_Sm_Debug; }

// --------------------------------------------------------------------------------------------------------------------

class FCkHfsmViewer_DataCollector
{
public:
    auto
    Collect(
        UWorld* InWorld) -> void;

    auto
    Get_AllStateMachines() const -> const TArray<FCkHfsmViewer_SmInfo>&;

private:
    auto
    CollectStateMachine(
        const FCk_Handle& InSmHandle) -> FCkHfsmViewer_SmInfo;

    auto
    OverlayLiveData(
        const FCk_Handle& InStateHandle,
        int32 InCurrentStateIndex,
        const TMap<TSubclassOf<UCk_SmState_EntityScript>, int32>& InStateClassToIndex,
        FCkHfsmViewer_SmInfo& InOutSmInfo) -> void;

private:
    TArray<FCkHfsmViewer_SmInfo> _StateMachines;
};

// --------------------------------------------------------------------------------------------------------------------
