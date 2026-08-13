// A/B evidence for the registry-teardown use-after-free. OPT-IN ONLY — the unguarded leg is
// designed to kill the process, so it self-skips unless CK_UAF_REPRO=1 is set in the environment.
//
//   Run the pair:  $env:CK_UAF_REPRO='1'; ./CkAuto/UnrealToolbox.exe --test --test-pattern TeardownUafRepro ...
//
// What this proves that the sibling CkRegistry_Teardown.spec.cpp cannot: that the branch at
// CkSignal_Fragment.inl.h ~line 83 is what stands between this project and the field crash.
// The window spec asserts the CONTRACT (the window opens during real ~FEcsWorld teardown); this
// asserts the CONSEQUENCE (taking the release path against a dead sigh is fatal; skipping it is not).
//
// Fidelity note, stated plainly: the sigh is hosted in a buffer this spec owns and is poisoned
// after destruction, so the fault is deterministic instead of depending on whether the freed page
// happens to still be mapped — which is exactly why editor builds survived this bug for three
// months while packaged builds died. The destructor under test is the REAL production one; only
// the sigh's storage is the spec's. The field evidence that this path is genuinely taken is the
// symbolicated Sentry dump (entt::sink::release <- ~TFragment_Signal_Delegate, AV 0xC0000005).

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Registry/CkRegistry_Teardown.h"
#include "CkEcs/Signal/CkSignal_Fragment.h"

#include "CoreMinimal.h"
#include "HAL/PlatformMisc.h"
#include "Misc/AutomationTest.h"

#include <entt/signal/sigh.hpp>
#include <new>

#if WITH_DEV_AUTOMATION_TESTS

namespace ck
{
    // Befriended by TFragment_Signal_Delegate (dev-automation builds only).
    struct FSignalDelegate_TeardownSpecAccess
    {
        template <typename T_Fragment>
        static auto Set_Connection(T_Fragment& InFragment, entt::connection InConnection) -> void
        {
            InFragment._Connection = InConnection;
        }
    };
}

namespace ck_teardown_uaf_repro
{
    using FSighType = entt::sigh<void()>;

    // entt::connection is NOT templated on the signal signature, so a void() sigh drives the real
    // fragment's connection member exactly as a gameplay signal's would.
    using FDelegateFragment = ck::FFragment_Signal_Delegate_OnEntityBeginDestroy;

    static auto Noop() -> void {}

    static auto Get_IsReproEnabled() -> bool
    {
        return FPlatformMisc::GetEnvironmentVariable(TEXT("CK_UAF_REPRO")) == TEXT("1");
    }

    // Builds a real fragment whose connection points into a sigh we own, then destroys that sigh
    // and poisons its storage — reproducing "the sigh's pool died first".
    struct FPoisonedSighScenario
    {
    public:
        FPoisonedSighScenario()
        {
            _Sigh = new (_Buffer) FSighType{};
            auto Sink = entt::sink<FSighType>{*_Sigh};
            ck::FSignalDelegate_TeardownSpecAccess::Set_Connection(_Fragment, Sink.connect<&Noop>());
        }

        FPoisonedSighScenario(const FPoisonedSighScenario&) = delete;
        auto operator=(const FPoisonedSighScenario&) -> FPoisonedSighScenario& = delete;

        auto Kill_Sigh() -> void
        {
            _Sigh->~FSighType();
            _Sigh = nullptr;

            // Each 8-byte word gets a DISTINCT non-canonical value, and that detail is the whole
            // experiment. entt's disconnect walks `for(auto pos = ref.calls.size(); pos; --pos)`
            // (sigh.hpp:379). A uniform fill — or a zero fill — makes the vector's begin and end
            // compare EQUAL, so size() is 0, the loop never runs, and the release is harmless: the
            // first version of this spec memset 0xDD and did not crash for exactly that reason.
            // Varying the pattern models reused heap, where begin != end forces the indexing that
            // dereferences a garbage pointer — the field crash.
            //
            // Do NOT read the uniform-fill case as an explanation of why editor builds survived
            // this bug: UE_USE_MALLOC_FILL_BYTES (MallocPoisonProxy.h) 0xDD-fills freed memory in
            // Development NON-editor builds, i.e. the packaged one that crashed, and not the editor.
            // The explanation runs the wrong way round. Editor survival is unexplained.
            auto* Words = reinterpret_cast<uint64*>(&_Buffer[0]);
            const auto WordCount = sizeof(_Buffer) / sizeof(uint64);
            for (auto Index = 0u; Index < WordCount; ++Index)
            { Words[Index] = 0xDEAD000000001000ull + (static_cast<uint64>(Index) * 0x40ull); }
        }

        FDelegateFragment _Fragment;

    private:
        alignas(FSighType) uint8 _Buffer[sizeof(FSighType)] = {};
        FSighType* _Sigh = nullptr;
    };
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Signal_TeardownUafRepro,
    "Ck.CkEcs.Signal.TeardownUafRepro",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Signal_TeardownUafRepro::RunTest(const FString&)
{
    if (NOT ck_teardown_uaf_repro::Get_IsReproEnabled())
    {
        AddInfo(TEXT("SKIPPED — set CK_UAF_REPRO=1 to run. The unguarded leg deliberately crashes."));
        return true;
    }

    // ---- Leg B (the fix): destroy the fragment INSIDE the teardown window -------------------
    // Pre-fix this was an unconditional release into the poisoned sigh. Surviving to the next line
    // is the whole point. The scenario is scoped INSIDE the window so the fragment's destructor —
    // which runs last, after the poisoned buffer — fires while the window is open.
    // UE_LOG rather than AddInfo for the leg markers: the unguarded leg kills the process, which
    // discards the automation framework's buffered results. Without a flushed-to-log breadcrumb the
    // crash artifact cannot show WHICH leg died, and "it crashed somewhere in this test" is not an
    // A/B result.
    UE_LOG(LogTemp, Display, TEXT("[CkTeardownUafRepro] GUARDED leg starting (window OPEN)"));
    {
        const auto TeardownWindow = ck::registry_teardown::FScopedGuard{};

        auto Scenario = ck_teardown_uaf_repro::FPoisonedSighScenario{};
        Scenario.Kill_Sigh();
    }
    UE_LOG(LogTemp, Display, TEXT("[CkTeardownUafRepro] GUARDED leg SURVIVED — release skipped against a dead sigh"));
    AddInfo(TEXT("GUARDED leg survived: release was skipped against a dead sigh."));
    TestTrue(TEXT("guarded destruction survives a dead sigh"), true);

    // ---- Leg A (pre-fix behaviour): destroy it OUTSIDE the window ----------------------------
    // The window closed is byte-for-byte what the code did before the fix. Expect an access
    // violation here; the process dies and the automation log's tail is the A-side artifact.
    AddInfo(TEXT("UNGUARDED leg starting — an access violation HERE is the expected A-side result."));
    UE_LOG(LogTemp, Display,
        TEXT("[CkTeardownUafRepro] UNGUARDED leg starting (window CLOSED) — an ACCESS VIOLATION here is the expected pre-fix result; the process is meant to die"));
    GLog->Flush();
    {
        auto Scenario = ck_teardown_uaf_repro::FPoisonedSighScenario{};
        Scenario.Kill_Sigh();
    }   // ~FDelegateFragment with the window CLOSED -> _Connection.release() -> AV

    UE_LOG(LogTemp, Error, TEXT("[CkTeardownUafRepro] UNGUARDED leg did NOT crash"));

    AddError(TEXT("UNGUARDED leg did NOT crash — the poisoned sigh was read without faulting, so "
                  "this run proves nothing about the A-side. Do not read it as evidence."));
    return false;
}

#endif
