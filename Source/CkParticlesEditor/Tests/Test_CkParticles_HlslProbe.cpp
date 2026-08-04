#include "CkParticles/ScriptDefinition/CkParticles_ScriptDefinition_Naming.h"
#include "CkParticles_Log.h"

#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "NiagaraEmitter.h"
#include "NiagaraScript.h"
#include "NiagaraSystem.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------
// Diagnostic lane for the VFX-select PSO freeze: reports, per GPU sim script of two named templates, whether the
// persisted/DDC-fetched translated HLSL is the BAKED per-behavior dispatch or the LEGACY full 47-behavior corpus,
// then FORCES a fresh recompile of the slow template (FireBurst) so [HlslCodegen] in the data interface logs which
// object and which id set the live translation actually consulted. PickupCast is deliberately never force-recompiled:
// its DDC entry is the one known-fast artifact and a forced rewrite could regress it.
//
// Env-gated so the default test pass never recompiles anything: set CK_PARTICLES_HLSL_PROBE=1, then run the toolbox
// with --test --no-nullrhi --test-pattern HlslProbe --discover-fresh. Non-empty translations are written to
// Saved/CkParticles_HlslProbe/ for diffing.
// --------------------------------------------------------------------------------------------------------------------

namespace ck_particles_hlsl_probe_test
{
    auto Get_IsSystemReady(UNiagaraSystem* InSystem) -> bool
    {
#if WITH_EDITORONLY_DATA
        if (NOT InSystem->HasOutstandingCompilationRequests() && InSystem->IsReadyToRun() && NOT InSystem->NeedsRequestCompile())
        { return true; }

        // Self-driving, like the prewarm lane: the automation editor context runs no world-tick Niagara poll.
        InSystem->PollForCompilationComplete();

        return NOT InSystem->HasOutstandingCompilationRequests() && InSystem->IsReadyToRun() && NOT InSystem->NeedsRequestCompile();
#else
        return InSystem->IsReadyToRun();
#endif
    }

    auto Do_DumpGpuScripts(FAutomationTestBase* InTest, UNiagaraSystem* InSystem, const FString& InLabel) -> void
    {
#if WITH_EDITORONLY_DATA
        for (const FNiagaraEmitterHandle& EmitterHandle : InSystem->GetEmitterHandles())
        {
            const auto* EmitterData = EmitterHandle.GetEmitterData();
            if (EmitterData == nullptr || EmitterData->SimTarget != ENiagaraSimTarget::GPUComputeSim)
            { continue; }

            auto* GpuScript = EmitterData->GetGPUComputeScript();
            if (GpuScript == nullptr)
            { continue; }

            const auto& Hlsl = GpuScript->GetVMExecutableData().LastHlslTranslationGPU;
            const auto HasLegacyMarker = Hlsl.Contains(TEXT("CkParticles_Behaviors.ush"));
            const auto HasBakedMarker  = Hlsl.Contains(TEXT("CkParticles_BakedDispatch"));

            InTest->AddInfo(FString::Printf(
                TEXT("[HlslProbe][%s] %s/%s: hlslLen=%d legacyMarker=%d bakedMarker=%d shaderSync=%d"),
                *InLabel, *InSystem->GetName(), *EmitterHandle.GetName().ToString(),
                Hlsl.Len(), HasLegacyMarker ? 1 : 0, HasBakedMarker ? 1 : 0,
                GpuScript->IsScriptShaderSynchronized() ? 1 : 0));
            UE_LOG(LogTemp, Display, TEXT("[HlslProbe][%s] %s/%s: hlslLen=%d legacyMarker=%d bakedMarker=%d"),
                *InLabel, *InSystem->GetName(), *EmitterHandle.GetName().ToString(),
                Hlsl.Len(), HasLegacyMarker ? 1 : 0, HasBakedMarker ? 1 : 0);

            if (Hlsl.Len() > 0)
            {
                const auto OutPath = FPaths::ProjectSavedDir() / TEXT("CkParticles_HlslProbe") /
                    FString::Printf(TEXT("%s_%s_%s.hlsl"), *InLabel, *InSystem->GetName(), *EmitterHandle.GetName().ToString());
                FFileHelper::SaveStringToFile(Hlsl, *OutPath);
                InTest->AddInfo(FString::Printf(TEXT("[HlslProbe] wrote [%s]"), *OutPath));
            }
        }
#endif
    }

    class FCk_Latent_HlslProbe : public IAutomationLatentCommand
    {
    public:
        FCk_Latent_HlslProbe(
                FAutomationTestBase* InTest,
                TStrongObjectPtr<UNiagaraSystem> InSlowSystem,
                TStrongObjectPtr<UNiagaraSystem> InFastSystem,
                double InTimeoutSeconds)
            : _Test(InTest)
            , _SlowSystem(MoveTemp(InSlowSystem))
            , _FastSystem(MoveTemp(InFastSystem))
            , _TimeoutSeconds(InTimeoutSeconds)
        {
        }

        virtual ~FCk_Latent_HlslProbe() = default;
        virtual bool Update() override;

    private:
        enum class EStage { WaitInitial, WaitForced };

        FAutomationTestBase*             _Test = nullptr;
        TStrongObjectPtr<UNiagaraSystem> _SlowSystem;
        TStrongObjectPtr<UNiagaraSystem> _FastSystem;
        double                           _TimeoutSeconds = 300.0;
        double                           _StartTime = -1.0;
        double                           _LastHeartbeatTime = -1.0;
        int32                            _TicksInForcedStage = 0;
        EStage                           _Stage = EStage::WaitInitial;
    };

    bool
        FCk_Latent_HlslProbe::
        Update()
    {
        if (_StartTime < 0.0)
        { _StartTime = FPlatformTime::Seconds(); }

        const auto Elapsed = FPlatformTime::Seconds() - _StartTime;

        auto* SlowSystem = _SlowSystem.Get();
        auto* FastSystem = _FastSystem.Get();
        if (NOT IsValid(SlowSystem) || NOT IsValid(FastSystem))
        {
            _Test->AddError(TEXT("probe systems were invalidated mid-wait"));
            return true;
        }

        if (_Stage == EStage::WaitInitial)
        {
            if (Get_IsSystemReady(SlowSystem) && Get_IsSystemReady(FastSystem))
            {
                Do_DumpGpuScripts(_Test, SlowSystem, TEXT("PhaseA"));
                Do_DumpGpuScripts(_Test, FastSystem, TEXT("PhaseA"));

                constexpr auto ForceRecompile = true;
                SlowSystem->RequestCompile(ForceRecompile);
                _Test->AddInfo(FString::Printf(TEXT("[HlslProbe] forced recompile requested for [%s]"), *SlowSystem->GetName()));

                _Stage = EStage::WaitForced;
                return false;
            }
        }
        else
        {
            // A few settle ticks so a just-issued request registers before readiness is trusted again.
            ++_TicksInForcedStage;
            if (_TicksInForcedStage > 5 && Get_IsSystemReady(SlowSystem))
            {
                Do_DumpGpuScripts(_Test, SlowSystem, TEXT("PostForce"));
                return true;
            }
        }

        if (_LastHeartbeatTime < 0.0 || (FPlatformTime::Seconds() - _LastHeartbeatTime) > 15.0)
        {
            _LastHeartbeatTime = FPlatformTime::Seconds();
            UE_LOG(LogTemp, Display, TEXT("[HlslProbe] %.0fs elapsed — stage %d"), Elapsed, static_cast<int32>(_Stage));
        }

        if (Elapsed < _TimeoutSeconds)
        { return false; }

        _Test->AddError(FString::Printf(TEXT("timed out after %.0fs in stage %d"), _TimeoutSeconds, static_cast<int32>(_Stage)));
        return true;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCk_Particles_HlslProbe_Test,
    "Ck.Particles.HlslProbe",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCk_Particles_HlslProbe_Test::RunTest(const FString& Parameters)
{
    using namespace ck_particles_hlsl_probe_test;

    if (FPlatformMisc::GetEnvironmentVariable(TEXT("CK_PARTICLES_HLSL_PROBE")) != TEXT("1"))
    {
        AddInfo(TEXT("Skipped — set CK_PARTICLES_HLSL_PROBE=1 to run the translated-HLSL probe."));
        return true;
    }

    constexpr auto FireBurstBehaviorId  = 20;
    constexpr auto PickupCastBehaviorId = 30;

    const auto SlowPath = ck::particles::Get_BehaviorTemplateSystemObjectPath(FireBurstBehaviorId);
    const auto FastPath = ck::particles::Get_BehaviorTemplateSystemObjectPath(PickupCastBehaviorId);

    auto SlowSystem = TStrongObjectPtr{LoadObject<UNiagaraSystem>(nullptr, *SlowPath)};
    auto FastSystem = TStrongObjectPtr{LoadObject<UNiagaraSystem>(nullptr, *FastPath)};

    TestNotNull(*FString::Printf(TEXT("slow template loads [%s]"), *SlowPath), SlowSystem.Get());
    TestNotNull(*FString::Printf(TEXT("fast template loads [%s]"), *FastPath), FastSystem.Get());
    if (SlowSystem.Get() == nullptr || FastSystem.Get() == nullptr)
    { return false; }

    AddInfo(FString::Printf(TEXT("[HlslProbe] at load: %s needsCompile=%d, %s needsCompile=%d"),
        *SlowSystem->GetName(), SlowSystem->NeedsRequestCompile() ? 1 : 0,
        *FastSystem->GetName(), FastSystem->NeedsRequestCompile() ? 1 : 0));

    constexpr auto TimeoutSeconds = 300.0;
    ADD_LATENT_AUTOMATION_COMMAND(FCk_Latent_HlslProbe(this, MoveTemp(SlowSystem), MoveTemp(FastSystem), TimeoutSeconds));

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
