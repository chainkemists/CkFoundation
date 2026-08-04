#include "CkCore/Macros/CkMacros.h"
#include "CkParticles/ScriptDefinition/CkParticles_ScriptDefinition_Naming.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/IConsoleManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/PackageName.h"
#include "NiagaraEmitter.h"
#include "NiagaraSystem.h"
#include "UObject/SavePackage.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------
// Prewarm/stabilize lane: load every distinct behavior template plus every original Niagara system under
// /Game/Vefects, and hold the run open until each has finished compiling.
//
// Every system that arrives with NeedsRequestCompile() true was OUT OF SYNC ON DISK: UNiagaraSystem::PostLoad
// re-checks AreScriptAndSourceSynchronized() on every load, and load-time graph fixups bump ChangeIds that the
// saved asset predates — so such an asset recompiles in EVERY editor session (the "Preparing... Niagara Systems"
// toast and the first-activation stall) until its post-fixup compile ids and VM data are saved back. That resave is
// what CK_PARTICLES_STABILIZE=1 does, mirroring the engine's own resave-commandlet path (NiagaraSystem.cpp's
// GMarkPackageDirtyWhenNotSynchronized). The default (no env) run mutates nothing — it only prewarms the DDC and
// reports the out-of-sync count, which a post-stabilize run must report as 0.
//
// Ritual after a template regen: toolbox --test --no-nullrhi --test-pattern PrewarmTemplates with
// CK_PARTICLES_STABILIZE=1, then once more without the env var to verify "out-of-sync at load: 0".
// --------------------------------------------------------------------------------------------------------------------

namespace ck_particles_prewarm_test
{
    auto Get_IsSystemReady(UNiagaraSystem* InSystem) -> bool
    {
#if WITH_EDITORONLY_DATA
        // The same two questions UCk_Utils_Particles_UE::Get_IsBehaviorTemplateReady asks — VM only, and for the
        // same reason (see that function): the GPU flag never converges on a loaded-but-never-activated system.
        if (NOT InSystem->HasOutstandingCompilationRequests() && InSystem->IsReadyToRun() && NOT InSystem->NeedsRequestCompile())
        { return true; }

        // Self-driving, like the util: the automation editor context runs no world-tick Niagara poll, so a
        // deferred RequestCompile is never issued and finished compiles are never finalized without this
        // (measured 2026-08-03: all 32 templates sat NeedsRequestCompile for the full 600 s timeout).
        InSystem->PollForCompilationComplete();

        return NOT InSystem->HasOutstandingCompilationRequests() && InSystem->IsReadyToRun() && NOT InSystem->NeedsRequestCompile();
#else
        return InSystem->IsReadyToRun();
#endif
    }

    // Mirrors the GPU leg of FVersionedNiagaraEmitterData::AreAllScriptAndSourcesSynchronized — the exact condition
    // the NEXT load will verify. Saving on VM readiness alone re-persists an asset whose GPU shader map is still
    // compiling, and that asset fails the load check forever (measured 2026-08-03: run 2 of the stabilize lane).
    auto Get_AreGpuShadersSynchronized(UNiagaraSystem* InSystem) -> bool
    {
#if WITH_EDITORONLY_DATA
        if (NOT UNiagaraScript::AreGpuScriptsCompiledBySystem())
        { return true; }

        for (const FNiagaraEmitterHandle& EmitterHandle : InSystem->GetEmitterHandles())
        {
            const auto* EmitterData = EmitterHandle.GetEmitterData();
            if (EmitterData == nullptr || EmitterData->SimTarget != ENiagaraSimTarget::GPUComputeSim)
            { continue; }

            auto* GpuScript = EmitterData->GetGPUComputeScript();
            if (GpuScript != nullptr && GpuScript->IsCompilable() && NOT GpuScript->IsScriptShaderSynchronized())
            { return false; }
        }
#endif
        return true;
    }

    // Per-script forensic dump for a system whose load-time check refuses to synchronize: names WHICH script fails
    // and whether it is an id mismatch, an invalid computed id, or missing compiled data — the three cases the
    // engine's verbose logging cannot distinguish (its per-script lines are gated on valid cached VM data).
    auto Do_LogScriptSyncDetail(UNiagaraSystem* InSystem) -> void
    {
#if WITH_EDITORONLY_DATA
        const auto LogOneScript = [&](UNiagaraScript* InScript, const TCHAR* InLabel) -> void
        {
            if (InScript == nullptr || NOT InScript->IsCompilable())
            { return; }

            FNiagaraVMExecutableDataId NewId;
            InScript->ComputeVMCompilationId(NewId, FGuid{});
            const auto& CachedId = InScript->GetVMExecutableDataCompilationId();
            UE_LOG(LogTemp, Display,
                TEXT("[PrewarmTemplates][%s] %s: sync=%d idEqual=%d newIdValid=%d cachedIdValid=%d vmValid=%d baseEqual=%d newRefs=%d cachedRefs=%d"),
                *InSystem->GetName(), InLabel,
                InScript->AreScriptAndSourceSynchronized() ? 1 : 0,
                (NewId == CachedId) ? 1 : 0,
                NewId.IsValid() ? 1 : 0,
                CachedId.IsValid() ? 1 : 0,
                InScript->GetVMExecutableData().IsValid() ? 1 : 0,
                (NewId.BaseScriptCompileHash == CachedId.BaseScriptCompileHash) ? 1 : 0,
                NewId.ReferencedCompileHashes.Num(), CachedId.ReferencedCompileHashes.Num());
        };

        LogOneScript(InSystem->GetSystemSpawnScript(), TEXT("SystemSpawn"));
        LogOneScript(InSystem->GetSystemUpdateScript(), TEXT("SystemUpdate"));

        for (const FNiagaraEmitterHandle& EmitterHandle : InSystem->GetEmitterHandles())
        {
            auto* EmitterData = EmitterHandle.GetEmitterData();
            if (EmitterData == nullptr)
            { continue; }

            LogOneScript(EmitterData->SpawnScriptProps.Script, TEXT("EmitterSpawn"));
            LogOneScript(EmitterData->UpdateScriptProps.Script, TEXT("EmitterUpdate"));

            if (EmitterData->SimTarget == ENiagaraSimTarget::GPUComputeSim)
            {
                auto* GpuScript = EmitterData->GetGPUComputeScript();
                LogOneScript(GpuScript, TEXT("GpuCompute"));
                if (GpuScript != nullptr && UNiagaraScript::AreGpuScriptsCompiledBySystem())
                {
                    UE_LOG(LogTemp, Display, TEXT("[PrewarmTemplates][%s] GpuShaderSync=%d"),
                        *InSystem->GetName(), GpuScript->IsScriptShaderSynchronized() ? 1 : 0);
                }
            }
        }
#endif
    }

    auto Try_ResaveSystem(UNiagaraSystem* InSystem) -> bool
    {
        auto* Package = InSystem->GetPackage();
        InSystem->MarkPackageDirty();

        const auto FileName = FPackageName::LongPackageNameToFilename(
            Package->GetName(), FPackageName::GetAssetPackageExtension());
        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        return UPackage::SavePackage(Package, InSystem, *FileName, SaveArgs);
    }

    // Defined manually rather than via DEFINE_LATENT_AUTOMATION_COMMAND_*, which carries no mutable progress state —
    // this command tracks its own deadline and which systems it has already reported. Same shape as the CkTests
    // harness commands (CkTests/Net/CkNetAutomation_Common.h).
    class FCk_Latent_WaitForTemplateCompiles : public IAutomationLatentCommand
    {
    public:
        FCk_Latent_WaitForTemplateCompiles(
                FAutomationTestBase* InTest,
                TArray<TStrongObjectPtr<UNiagaraSystem>> InSystems,
                TArray<bool> InOutOfSyncAtLoad,
                bool InStabilize,
                double InTimeoutSeconds)
            : _Test(InTest)
            , _Systems(MoveTemp(InSystems))
            , _OutOfSyncAtLoad(MoveTemp(InOutOfSyncAtLoad))
            , _Stabilize(InStabilize)
            , _TimeoutSeconds(InTimeoutSeconds)
        {
            _Reported.Init(false, _Systems.Num());
        }

        virtual ~FCk_Latent_WaitForTemplateCompiles() = default;
        virtual bool Update() override;

    private:
        FAutomationTestBase*                     _Test = nullptr;
        TArray<TStrongObjectPtr<UNiagaraSystem>> _Systems;
        TArray<bool>                             _OutOfSyncAtLoad;
        TArray<bool>                             _Reported;
        bool                                     _Stabilize = false;
        double                                   _TimeoutSeconds = 600.0;
        double                                   _StartTime = -1.0;
        double                                   _LastHeartbeatTime = -1.0;
    };

    bool
        FCk_Latent_WaitForTemplateCompiles::
        Update()
    {
        if (_StartTime < 0.0)
        { _StartTime = FPlatformTime::Seconds(); }

        const auto Elapsed = FPlatformTime::Seconds() - _StartTime;
        auto StillCold = TArray<FString>{};

        for (auto Index = 0; Index < _Systems.Num(); ++Index)
        {
            auto* System = _Systems[Index].Get();
            if (NOT IsValid(System))
            { continue; }

            if (NOT Get_IsSystemReady(System))
            {
                StillCold.Add(System->GetName());
                continue;
            }

            // Stabilizing waits past VM readiness for the GPU shader maps, so the resave captures a state the
            // next load's synchronization check accepts in full.
            if (_Stabilize && NOT Get_AreGpuShadersSynchronized(System))
            {
                StillCold.Add(System->GetName() + TEXT("(gpu)"));
                continue;
            }

            if (_Reported[Index])
            { continue; }

            // One line per system as it lands: the toolbox streams this, so a long cold-DDC prewarm reads as
            // progress rather than as a hang, and its idle watchdog is fed.
            _Reported[Index] = true;
            _Test->AddInfo(FString::Printf(TEXT("compiled [%s] at %.1fs"), *System->GetName(), Elapsed));
        }

        if (StillCold.IsEmpty())
        {
            if (_Stabilize)
            {
                auto SavedCount = 0;
                for (auto Index = 0; Index < _Systems.Num(); ++Index)
                {
                    auto* System = _Systems[Index].Get();
                    if (NOT IsValid(System) || NOT _OutOfSyncAtLoad[Index])
                    { continue; }

                    if (Try_ResaveSystem(System))
                    { ++SavedCount; }
                    else
                    { _Test->AddError(FString::Printf(TEXT("failed to resave [%s]"), *System->GetPathName())); }
                }

                _Test->AddInfo(FString::Printf(TEXT("stabilized %d out-of-sync system(s) — resaved with post-fixup compile ids"), SavedCount));
                UE_LOG(LogTemp, Display, TEXT("[PrewarmTemplates] stabilized %d out-of-sync system(s)"), SavedCount);
            }

            return true;
        }

        // AddInfo only surfaces when the test COMPLETES, so a long compile wait is otherwise silent on the
        // toolbox's stream — and its idle watchdog killed exactly such a silent stretch once (2026-08-03).
        // A periodic Display line through a stock category keeps the run visibly alive.
        if (_LastHeartbeatTime < 0.0 || (FPlatformTime::Seconds() - _LastHeartbeatTime) > 15.0)
        {
            _LastHeartbeatTime = FPlatformTime::Seconds();
            UE_LOG(LogTemp, Display, TEXT("[PrewarmTemplates] %.0fs elapsed — %d/%d system(s) still compiling: [%s]"),
                Elapsed, StillCold.Num(), _Systems.Num(), *FString::Join(StillCold, TEXT(", ")));
        }

        // The engine polls every UNiagaraSystem's compilation on world tick start (INiagaraModule::OnWorldTickStart),
        // and the editor ticks between latent Update() calls — so returning false is what advances the compiles.
        if (Elapsed < _TimeoutSeconds)
        { return false; }

        _Test->AddError(FString::Printf(TEXT("timed out after %.0fs with %d system(s) still compiling: [%s]"),
            _TimeoutSeconds, StillCold.Num(), *FString::Join(StillCold, TEXT(", "))));

        return true;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCk_Particles_PrewarmTemplates_Test,
    "Ck.Particles.PrewarmTemplates",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCk_Particles_PrewarmTemplates_Test::RunTest(const FString& Parameters)
{
    const auto Stabilize = FPlatformMisc::GetEnvironmentVariable(TEXT("CK_PARTICLES_STABILIZE")) == TEXT("1");
    if (Stabilize)
    {
        // Makes the engine log WHICH script ChangeId desynchronized each system — the evidence for what the
        // resave is fixing lands in this run's log.
        if (auto* Cvar = IConsoleManager::Get().FindConsoleVariable(TEXT("fx.EnableVerboseNiagaraChangeIdLogging")))
        { Cvar->Set(1); }
    }

    auto Systems = TArray<TStrongObjectPtr<UNiagaraSystem>>{};
    auto OutOfSyncAtLoad = TArray<bool>{};
    auto OutOfSyncNames = TArray<FString>{};

    const auto LoadAndRecord = [&](const FString& InPath) -> void
    {
        // The load is what kicks the compile request; the strong pointer keeps it kicked for the whole wait.
        auto* System = LoadObject<UNiagaraSystem>(nullptr, *InPath);
        if (NOT TestNotNull(*FString::Printf(TEXT("system loads [%s]"), *InPath), System))
        { return; }

        // True here means PostLoad found the saved asset out of sync with its (fixed-up) source graphs — this
        // asset recompiles every session until stabilized.
        const auto NeedsCompile = System->NeedsRequestCompile();
        if (NeedsCompile)
        {
            OutOfSyncNames.Add(System->GetName());
            constexpr auto MaxForensicDumps = 2;
            if (Stabilize && OutOfSyncNames.Num() <= MaxForensicDumps)
            { ck_particles_prewarm_test::Do_LogScriptSyncDetail(System); }
        }

        Systems.Add(TStrongObjectPtr<UNiagaraSystem>{System});
        OutOfSyncAtLoad.Add(NeedsCompile);
    };

    // Driven off the roster, and DEDUPED: a cadence row serves several behaviors (the projectile pair, the palette
    // twins) and most of the roster shares the seed and burst templates, so the loaded set is one per template.
    auto SeenPaths = TSet<FString>{};
    for (auto BehaviorId = 0; BehaviorId < ck::particles::NumBehaviors; ++BehaviorId)
    {
        const auto Path = ck::particles::Get_BehaviorTemplateSystemObjectPath(BehaviorId);

        auto AlreadySeen = false;
        SeenPaths.Add(Path, &AlreadySeen);
        if (AlreadySeen)
        { continue; }

        LoadAndRecord(Path);
    }
    const auto TemplateCount = Systems.Num();

    // The gym's A/B pairs also activate the original Vefects systems — same per-session recompile, same stall.
    // Soft outside this host project: an empty scan just means there are no originals to warm.
    auto& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
    auto Filter = FARFilter{};
    Filter.PackagePaths.Add(TEXT("/Game/Vefects"));
    Filter.bRecursivePaths = true;
    Filter.ClassPaths.Add(UNiagaraSystem::StaticClass()->GetClassPathName());

    auto OriginalAssets = TArray<FAssetData>{};
    AssetRegistry.GetAssets(Filter, OriginalAssets);
    for (const auto& Asset : OriginalAssets)
    { LoadAndRecord(Asset.GetObjectPathString()); }

    AddInfo(FString::Printf(TEXT("prewarming %d system(s): %d distinct template(s) across %d behaviors + %d Vefects original(s)"),
        Systems.Num(), TemplateCount, ck::particles::NumBehaviors, OriginalAssets.Num()));

    // THE verification observable: a stabilized content set reports 0 here on a fresh load.
    AddInfo(FString::Printf(TEXT("out-of-sync at load: %d/%d%s%s"),
        OutOfSyncNames.Num(), Systems.Num(),
        OutOfSyncNames.IsEmpty() ? TEXT("") : TEXT(" — "),
        *FString::Join(OutOfSyncNames, TEXT(", "))));
    UE_LOG(LogTemp, Display, TEXT("[PrewarmTemplates] out-of-sync at load: %d/%d"),
        OutOfSyncNames.Num(), Systems.Num());

    constexpr auto TimeoutSeconds = 600.0;
    ADD_LATENT_AUTOMATION_COMMAND(ck_particles_prewarm_test::FCk_Latent_WaitForTemplateCompiles(
        this, MoveTemp(Systems), MoveTemp(OutOfSyncAtLoad), Stabilize, TimeoutSeconds));

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
