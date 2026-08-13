// A/B evidence for the signal-teardown use-after-free and the contract that prevents it:
// TFragment_Signal_Delegate performs NO release on destruction (see the _Connection contract in
// CkSignal_Fragment.h). The sigh a connection points at lives in a different pool; during
// ~basic_registry that pool dies first, so any destructor-side release reads freed memory — the
// packaged save/load crash (symbolicated Sentry dump: entt::sink::release <-
// ~TFragment_Signal_Delegate, AV 0xC0000005).
//
//   Leg B (contract, ALWAYS runs): destroy a real fragment whose connection points into a
//   destroyed, poisoned sigh. The implicit destructor touches nothing, so this survives. If a
//   destructor-side release is ever reintroduced, THIS LEG CRASHES THE SUITE — that is the
//   tripwire, not an accident.
//
//   Leg A (the prevented crash, OPT-IN via CK_UAF_REPRO=1): explicitly release the same
//   connection into the poisoned sigh — byte-for-byte what the pre-fix destructor did. The
//   process is meant to die here.
//
//   Run the pair:  $env:CK_UAF_REPRO='1'; ./CkAuto/UnrealToolbox.exe --test --test-pattern TeardownUafRepro ...
//
// Fidelity note, stated plainly: the sigh is hosted in a buffer this spec owns and is poisoned
// after destruction, so the fault is deterministic instead of depending on whether the freed page
// happens to still be mapped — which is exactly why editor builds survived this bug for three
// months while packaged builds died. The fragment and its connection are the REAL production
// types; only the sigh's storage is the spec's.

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
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

        // Leg A only: re-enacts the pre-fix destructor body against the poisoned sigh.
        template <typename T_Fragment>
        static auto Release_Connection(T_Fragment& InFragment) -> void
        {
            InFragment._Connection.release();
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
    // ---- Leg B (the contract): destruction performs no release ------------------------------
    // The scenario's fragment — a REAL TFragment_Signal_Delegate holding a REAL connection into
    // the poisoned dead sigh — is destroyed at scope end. The implicit destructor must not touch
    // the connection; surviving to the next line is the whole point. A reintroduced destructor
    // release crashes HERE, in every ordinary suite run, which is deliberate.
    // UE_LOG rather than AddInfo for the leg markers: a crash discards the automation framework's
    // buffered results, so without a flushed-to-log breadcrumb the artifact cannot show WHICH leg
    // died, and "it crashed somewhere in this test" is not an A/B result.
    UE_LOG(LogTemp, Display, TEXT("[CkTeardownUafRepro] CONTRACT leg starting — fragment destruction against a dead sigh"));
    {
        auto Scenario = ck_teardown_uaf_repro::FPoisonedSighScenario{};
        Scenario.Kill_Sigh();
    }
    UE_LOG(LogTemp, Display, TEXT("[CkTeardownUafRepro] CONTRACT leg SURVIVED — destruction released nothing"));
    AddInfo(TEXT("CONTRACT leg survived: fragment destruction performed no release against a dead sigh."));
    TestTrue(TEXT("fragment destruction never releases its connection"), true);

    if (NOT ck_teardown_uaf_repro::Get_IsReproEnabled())
    {
        AddInfo(TEXT("CRASH leg SKIPPED — set CK_UAF_REPRO=1 to run it. It deliberately kills the process."));
        return true;
    }

    // ---- Leg A (the prevented crash): explicit release into the dead sigh -------------------
    // Byte-for-byte what the pre-fix destructor did. Expect an access violation here; the process
    // dies and the automation log's tail is the A-side artifact.
    AddInfo(TEXT("CRASH leg starting — an access violation HERE is the expected A-side result."));
    UE_LOG(LogTemp, Display,
        TEXT("[CkTeardownUafRepro] CRASH leg starting — explicit release into the dead sigh; an ACCESS VIOLATION here is the expected pre-fix result; the process is meant to die"));
    GLog->Flush();
    {
        auto Scenario = ck_teardown_uaf_repro::FPoisonedSighScenario{};
        Scenario.Kill_Sigh();
        ck::FSignalDelegate_TeardownSpecAccess::Release_Connection(Scenario._Fragment);
    }

    UE_LOG(LogTemp, Error, TEXT("[CkTeardownUafRepro] CRASH leg did NOT crash"));

    AddError(TEXT("CRASH leg did NOT crash — the poisoned sigh was read without faulting, so "
                  "this run proves nothing about the A-side. Do not read it as evidence."));
    return false;
}

#endif
