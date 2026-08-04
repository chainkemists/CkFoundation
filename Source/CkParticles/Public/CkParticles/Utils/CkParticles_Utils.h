#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "CkParticles_Utils.generated.h"

class UCkParticles_TuningDefinition;
class UNiagaraComponent;
class UNiagaraSystem;
struct FCkParticles_PartTuningBlock;

// --------------------------------------------------------------------------------------------------------------------
// Runtime spawn helpers: they wrap the standard Niagara spawn path and set User.BehaviorId. The behavior roster
// and each behavior's aim-axis convention live in CkParticles/CLAUDE.md.
// --------------------------------------------------------------------------------------------------------------------
UCLASS()
class CKPARTICLES_API UCk_Utils_Particles_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Size of the behavior roster; valid ids are [0, Get_NumBehaviors()-1]. Exposed so tests and gyms iterate every
    // behavior without each restating a maximum id that then drifts when a behavior is added.
    UFUNCTION(BlueprintPure, Category = "Ck|Particles",
        DisplayName = "[Ck][Particles] Get Num Behaviors")
    static int32
    Get_NumBehaviors();

    // Whether the behavior's template has finished compiling and will actually render if spawned now. In the editor
    // a freshly regenerated template compiles its Niagara scripts on first load — this function LOADS it, which is
    // what kicks that compile, and then reports the status — so a caller that can afford to wait (the VfxExamples
    // gym) polls here instead of spawning into an unfinished compile and showing nothing. A template that cannot be
    // loaded answers true: there is nothing to wait for, and the spawn path diagnoses the miss on its own.
    UFUNCTION(BlueprintPure, Category = "Ck|Particles",
        DisplayName = "[Ck][Particles] Get Is Behavior Template Ready")
    static bool
    Get_IsBehaviorTemplateReady(
        int32 InBehaviorId);

    // Spawns the CkParticles seed template (PS_CkParticles_Template) at a world location, selecting the behavior by
    // Id via User.BehaviorId. Returns the spawned component (nullptr if the template is missing or spawn fails).
    UFUNCTION(BlueprintCallable, Category = "Ck|Particles",
        meta = (WorldContext = "InWorldContextObject"),
        DisplayName = "[Ck][Particles] Spawn Behavior At Location")
    static UNiagaraComponent*
    Spawn_BehaviorAtLocation(
        const UObject* InWorldContextObject,
        int32 InBehaviorId,
        FVector InLocation,
        FRotator InRotation = FRotator::ZeroRotator,
        FVector InScale = FVector(1.f),
        FName InTextureName = NAME_None);

    // Spawns an explicit CkParticles system (e.g. a generated PS_CkParticles_<Name>) instead of the seed template,
    // still selecting the behavior by Id. Use when you have a per-effect generated asset.
    UFUNCTION(BlueprintCallable, Category = "Ck|Particles",
        meta = (WorldContext = "InWorldContextObject"),
        DisplayName = "[Ck][Particles] Spawn System At Location")
    static UNiagaraComponent*
    Spawn_SystemAtLocation(
        const UObject* InWorldContextObject,
        UNiagaraSystem* InSystem,
        int32 InBehaviorId,
        FVector InLocation,
        FRotator InRotation = FRotator::ZeroRotator,
        FVector InScale = FVector(1.f),
        FName InTextureName = NAME_None);

    // Spawn_BehaviorAtLocation, then apply InTuning to the spawned component. An explicit InTuning WINS over the
    // behavior's per-behavior tuning DataAsset, which the spawn path resolves by convention path; a null InTuning
    // leaves that asset's values in force (and, with no asset on disk, the identity).
    UFUNCTION(BlueprintCallable, Category = "Ck|Particles",
        meta = (WorldContext = "InWorldContextObject"),
        DisplayName = "[Ck][Particles] Spawn Behavior At Location (Tuned)")
    static UNiagaraComponent*
    Spawn_BehaviorAtLocation_Tuned(
        const UObject* InWorldContextObject,
        int32 InBehaviorId,
        FVector InLocation,
        FRotator InRotation = FRotator::ZeroRotator,
        FVector InScale = FVector(1.f),
        FName InTextureName = NAME_None,
        const UCkParticles_TuningDefinition* InTuning = nullptr);

    // Retunes a live CkParticles component: the asset's global floats become User.CkTuning, and its part rows become
    // the per-part block. A null InTuning writes the identity AND drops the part block, which is how a caller clears
    // an earlier tuning without needing an identity asset on disk.
    //
    // The part rows need the behavior id, which this reads back off the component's User.BehaviorId — so they reach
    // only components the CkParticles spawn path produced. The global floats reach any Niagara component.
    UFUNCTION(BlueprintCallable, Category = "Ck|Particles",
        DisplayName = "[Ck][Particles] Request Apply Tuning")
    static void
    Request_ApplyTuning(
        UNiagaraComponent* InComponent,
        const UCkParticles_TuningDefinition* InTuning);

    // Request_ApplyTuning's GLOBAL half from raw values, for callers with no tuning asset. It writes User.CkTuning
    // and nothing else — an existing part block is left exactly as it was.
    UFUNCTION(BlueprintCallable, Category = "Ck|Particles",
        DisplayName = "[Ck][Particles] Request Apply Tuning Values")
    static void
    Request_ApplyTuningValues(
        UNiagaraComponent* InComponent,
        float InSizeMultiplier = 1.f,
        float InColorIntensity = 1.f,
        float InAlphaMultiplier = 1.f,
        float InPlaybackSpeed = 1.f);

public:
    // Hangs a PER-PART tuning block on a live component: User.CkTuning tunes the whole system, this tunes the
    // layers inside it, addressed by the VisTag each behavior writes. The block reaches the stage through the DI's
    // per-instance data rather than a User parameter — Niagara has no float4-array user type.
    //
    // C++-facing on purpose: a UFUNCTION taking a 120-float4 struct is a surface nobody could author against. The
    // reflected way to reach it is a UCkParticles_TuningDefinition's part rows, through Request_ApplyTuning.
    static auto
    Request_ApplyPartTuningBlock(
        UNiagaraComponent*                  InComponent,
        const FCkParticles_PartTuningBlock& InBlock) -> void;

    // Drops the component's block. The system then renders at the identity — exactly as it does with no block at
    // all, which is the same thing.
    static auto
    Request_ResetPartTuning(
        UNiagaraComponent* InComponent) -> void;

private:
    // The ONE place the four tuning values become the User.CkTuning float4, so the asset path and the raw-value
    // path can never pack them in a different order.
    static FVector4
    DoPack_Tuning(
        float InSizeMultiplier,
        float InColorIntensity,
        float InAlphaMultiplier,
        float InPlaybackSpeed);
};
