#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"

#include "CkAngelscriptGenerator_DriftCommandlet.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * CI guardrail for AS bootstrap self-heal generators. Runs EntitySpawnParams,
 * AutoTestActors, and DynamicHandleTypes.json headlessly and exits 0;
 * drift surfaces via the surrounding CI's `git diff --exit-code` step.
 *
 *   UnrealEditor-Cmd.exe <Project.uproject> -run=CkAngelscriptGeneratorDrift
 *
 * AssetRegistry isn't covered — its generator is async (chained
 * RequestAsyncLoad) and would need engine ticks to complete inside the
 * commandlet. Editor-time AR-change listener keeps `*Assets.as` in sync
 * during normal development.
 */
UCLASS()
class CKANGELSCRIPTGENERATOR_API UCkAngelscriptGenerator_DriftCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UCkAngelscriptGenerator_DriftCommandlet();

    virtual int32 Main(const FString& Params) override;
};

// --------------------------------------------------------------------------------------------------------------------
