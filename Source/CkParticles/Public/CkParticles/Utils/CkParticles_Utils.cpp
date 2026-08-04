#include "CkParticles/Utils/CkParticles_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkParticles/DataInterface/CkParticles_PartTuning.h"
#include "CkParticles/ScriptDefinition/CkParticles_ScriptDefinition_Naming.h"
#include "CkParticles/ScriptDefinition/CkParticles_TuningDefinition.h"

#include "Engine/Engine.h"
#include "Engine/World.h"

#include "Materials/MaterialInterface.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CkParticles_Utils)

// --------------------------------------------------------------------------------------------------------------------

int32
    UCk_Utils_Particles_UE::
    Get_NumBehaviors()
{
    return ck::particles::NumBehaviors;
}

bool
    UCk_Utils_Particles_UE::
    Get_IsBehaviorTemplateReady(
        int32 InBehaviorId)
{
    const auto TemplatePath = ck::particles::Get_BehaviorTemplateSystemObjectPath(InBehaviorId);
    auto* System = LoadObject<UNiagaraSystem>(nullptr, *TemplatePath);

    if (NOT IsValid(System))
    { return true; }

#if WITH_EDITORONLY_DATA
    // VM/system scripts ONLY (the default). Deliberately NOT bIncludingGPUShaders=true: a compute script's shader
    // map is first REQUESTED by activation/rendering, so on a loaded-but-never-activated system that flag reads
    // not-finished forever with no compile in flight — a caller gating a spawn on it livelocks (measured
    // 2026-08-03). The VM compile is also the one that matters: activation blocks the game thread waiting on IT
    // (the minutes-long stall), while a cold compute shader just compiles async after activation — the effect
    // appears late, the editor stays live.
    if (NOT System->HasOutstandingCompilationRequests() && System->IsReadyToRun())
    { return true; }

    // Not ready — DRIVE the work this answer waits on, exactly as the engine's own WaitForCompilationComplete
    // loop does: a load can leave the system parked at NeedsRequestCompile (no request ever issued), and contexts
    // without the world-tick Niagara poll never issue or finalize it, so a poll-free check reads false forever
    // (measured 2026-08-03: all 32 templates, 600 s, zero compile activity).
    System->PollForCompilationComplete();

    return NOT System->HasOutstandingCompilationRequests() && System->IsReadyToRun();
#else
    return System->IsReadyToRun();
#endif
}

UNiagaraComponent*
    UCk_Utils_Particles_UE::
    Spawn_BehaviorAtLocation(
        const UObject* InWorldContextObject,
        int32          InBehaviorId,
        FVector        InLocation,
        FRotator       InRotation,
        FVector        InScale,
        FName          InTextureName)
{
    const auto TemplatePath = ck::particles::Get_BehaviorTemplateSystemObjectPath(InBehaviorId);
    auto* System = LoadObject<UNiagaraSystem>(nullptr, *TemplatePath);
    return Spawn_SystemAtLocation(InWorldContextObject, System, InBehaviorId, InLocation, InRotation, InScale, InTextureName);
}

UNiagaraComponent*
    UCk_Utils_Particles_UE::
    Spawn_BehaviorAtLocation_Tuned(
        const UObject*                       InWorldContextObject,
        int32                                InBehaviorId,
        FVector                              InLocation,
        FRotator                             InRotation,
        FVector                              InScale,
        FName                                InTextureName,
        const UCkParticles_TuningDefinition* InTuning)
{
    auto* Component = Spawn_BehaviorAtLocation(
        InWorldContextObject, InBehaviorId, InLocation, InRotation, InScale, InTextureName);

    if (NOT IsValid(Component)) { return Component; }

    // An explicit tuning OVERRIDES the behavior's convention asset, which the spawn path already applied. A null
    // one is not an identity request here — writing the identity would stomp that asset — so it falls through to
    // whatever the convention resolved.
    if (NOT IsValid(InTuning)) { return Component; }

    Request_ApplyTuning(Component, InTuning);

    return Component;
}

UNiagaraComponent*
    UCk_Utils_Particles_UE::
    Spawn_SystemAtLocation(
        const UObject* InWorldContextObject,
        UNiagaraSystem* InSystem,
        int32           InBehaviorId,
        FVector         InLocation,
        FRotator        InRotation,
        FVector         InScale,
        FName           InTextureName)
{
    if (NOT IsValid(InWorldContextObject)) { return nullptr; }
    if (NOT IsValid(InSystem)) { return nullptr; }

    auto* World = GEngine->GetWorldFromContextObject(InWorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
    if (NOT IsValid(World)) { return nullptr; }

    constexpr auto bAutoDestroy   = false;
    constexpr auto bAutoActivate  = true;
    constexpr auto PoolMethod     = ENCPoolMethod::None;
    constexpr auto bPreCullCheck  = true;

    auto* Component = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
        World, InSystem, InLocation, InRotation, InScale,
        bAutoDestroy, bAutoActivate, PoolMethod, bPreCullCheck);

    if (IsValid(Component))
    {
        Component->SetIntParameter(ck::particles::Get_BehaviorIdParameterName(), InBehaviorId);

        // The behavior's own tuning DataAsset, resolved by convention path. Absence is legitimate — a fresh
        // checkout runs before Generate_AllTuningAssets — and leaves the template's identity default in place,
        // so a miss is silent rather than diagnosed. An explicit tuning (Spawn_BehaviorAtLocation_Tuned) is
        // applied by that caller AFTER this, and wins.
        if (const auto TuningPath = ck::particles::Get_BehaviorTuningAssetObjectPath(InBehaviorId); NOT TuningPath.IsEmpty())
        {
            constexpr auto NoFilename = nullptr;
            constexpr auto QuietOnMiss = LOAD_NoWarn | LOAD_Quiet;
            if (auto* Tuning = LoadObject<UCkParticles_TuningDefinition>(nullptr, *TuningPath, NoFilename, QuietOnMiss))
            { Request_ApplyTuning(Component, Tuning); }
        }

        // Sprite material, in precedence order. Both write the SAME User.SpriteMaterial param, so a behavior with a
        // bound look renders it without callers knowing, and an explicit texture still wins when one is asked for.
        //
        // 1. A behavior-bound generated CkUsf look (the hand-authored-shader recreations).
        if (const auto LookName = ck::particles::Get_BehaviorLookName(InBehaviorId); LookName != NAME_None)
        {
            const auto LookPath = ck::particles::Get_GeneratedLookMasterObjectPath(LookName);
            if (auto* LookMaster = LoadObject<UMaterialInterface>(nullptr, *LookPath))
            { Component->SetVariableMaterial(ck::particles::Get_SpriteMaterialParameterName(), LookMaster); }
        }

        // 2. Per-effect procedural texture: swap the sprite material instance bound to the renderer's User param.
        //    Null-tolerant — a missing instance leaves the renderer's default (Glow) material, never invisible.
        if (InTextureName != NAME_None)
        {
            const auto MicPath = ck::particles::Get_TextureMaterialInstanceObjectPath(InTextureName);
            if (auto* Mic = LoadObject<UMaterialInterface>(nullptr, *MicPath))
            { Component->SetVariableMaterial(ck::particles::Get_SpriteMaterialParameterName(), Mic); }
        }
    }

    return Component;
}

void
    UCk_Utils_Particles_UE::
    Request_ApplyTuning(
        UNiagaraComponent*                   InComponent,
        const UCkParticles_TuningDefinition* InTuning)
{
    // A null definition is the identity — clearing a tuning is a request, not a mistake. An invalid component is
    // not: there is nothing to write to and the caller's intent is silently lost.
    const auto ComponentIsValid = IsValid(InComponent);
    CK_ENSURE_IF_NOT(ComponentIsValid, TEXT("Invalid Niagara Component supplied to Request_ApplyTuning"))
    {}
    if (NOT ComponentIsValid)
    { return; }

    if (NOT IsValid(InTuning))
    {
        Request_ApplyTuningValues(InComponent, 1.0f, 1.0f, 1.0f, 1.0f);

        // The part block lives in a module registry, not in a User parameter, so writing the identity float4 cannot
        // reach it: without this, clearing a tuning would leave every layer the previous asset moved still moved.
        Request_ResetPartTuning(InComponent);
        return;
    }

    Request_ApplyTuningValues(InComponent, InTuning->_SizeMultiplier, InTuning->_ColorIntensity,
        InTuning->_AlphaMultiplier, InTuning->_PlaybackSpeed);

    // Same reasoning from the other side: a tuning that declares no part rows IS the per-part identity, so applying
    // it must drop whatever an earlier one hung on the component.
    if (InTuning->_Parts.IsEmpty())
    {
        Request_ResetPartTuning(InComponent);
        return;
    }

    // Part rows are addressed against the behavior's OWN VisTag band, so the block cannot be built without the id.
    // The component carries it — the spawn path wrote User.BehaviorId before this ran — and reading it back keeps
    // the public signature, and every caller that already uses it, unchanged.
    auto       BehaviorIdIsSet = false;
    const auto BehaviorId      = InComponent->GetVariableInt(
        ck::particles::Get_BehaviorIdParameterName(), BehaviorIdIsSet);

    CK_ENSURE_IF_NOT(BehaviorIdIsSet, TEXT("Niagara Component [{}] carries no [{}], so the part rows of tuning [{}] ")
        TEXT("address no behavior. Part tuning only applies to components spawned through UCk_Utils_Particles_UE."),
        InComponent->GetName(), ck::particles::Get_BehaviorIdParameterName(), InTuning->GetName())
    {}
    if (NOT BehaviorIdIsSet)
    { return; }

    Request_ApplyPartTuningBlock(InComponent, InTuning->Get_AsPartTuningBlock(BehaviorId));
}

void
    UCk_Utils_Particles_UE::
    Request_ApplyTuningValues(
        UNiagaraComponent* InComponent,
        float              InSizeMultiplier,
        float              InColorIntensity,
        float              InAlphaMultiplier,
        float              InPlaybackSpeed)
{
    const auto ComponentIsValid = IsValid(InComponent);
    CK_ENSURE_IF_NOT(ComponentIsValid, TEXT("Invalid Niagara Component supplied to Request_ApplyTuningValues"))
    {}
    if (NOT ComponentIsValid)
    { return; }

    InComponent->SetVariableVec4(ck::particles::Get_TuningParameterName(),
        DoPack_Tuning(InSizeMultiplier, InColorIntensity, InAlphaMultiplier, InPlaybackSpeed));
}

auto
    UCk_Utils_Particles_UE::
    Request_ApplyPartTuningBlock(
        UNiagaraComponent*                  InComponent,
        const FCkParticles_PartTuningBlock& InBlock)
    -> void
{
    const auto ComponentIsValid = IsValid(InComponent);
    CK_ENSURE_IF_NOT(ComponentIsValid, TEXT("Invalid Niagara Component supplied to Request_ApplyPartTuningBlock"))
    {}
    if (NOT ComponentIsValid)
    { return; }

    ck::particles::Set_PartTuningBlock(InComponent, InBlock);
}

auto
    UCk_Utils_Particles_UE::
    Request_ResetPartTuning(
        UNiagaraComponent* InComponent)
    -> void
{
    const auto ComponentIsValid = IsValid(InComponent);
    CK_ENSURE_IF_NOT(ComponentIsValid, TEXT("Invalid Niagara Component supplied to Request_ResetPartTuning"))
    {}
    if (NOT ComponentIsValid)
    { return; }

    ck::particles::Remove_PartTuningBlock(InComponent);
}

FVector4
    UCk_Utils_Particles_UE::
    DoPack_Tuning(
        float InSizeMultiplier,
        float InColorIntensity,
        float InAlphaMultiplier,
        float InPlaybackSpeed)
{
    return FVector4(InSizeMultiplier, InColorIntensity, InAlphaMultiplier, InPlaybackSpeed);
}
