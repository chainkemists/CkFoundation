// Pins the registry-teardown window that TFragment_Signal_Delegate's destructor depends on.
//
// A signal's entt::sigh lives in TFragment_Signal; the entt::connection into it lives in
// TFragment_Signal_Delegate — a different fragment, in a different pool. entt's ~basic_registry is
// defaulted and pools die in insertion order, which Bind fixes as signal-then-delegate, so the sigh
// is freed FIRST every time and releasing the connection reads freed memory. That shipped as a
// packaged-build access violation on save/load (Ck_Load -> OpenLevel -> CleanupWorld ->
// ~basic_registry), symbolicated to entt::sink::release called from ~TFragment_Signal_Delegate.
//
// Both halves of the contract are load-bearing:
//   1. destructors running under whole-registry teardown MUST see the window open,
//   2. destructors running at any other time MUST NOT — otherwise the fix silently degrades into
//      "never disconnect" and leaks connections on ordinary fragment removal.
//
// Asserts the WINDOW rather than "did not crash". A dangling release only faults once the freed
// memory happens to hold something that makes the subscriber vector non-empty — it needs neither an
// unmapped page nor any particular allocator. So a no-crash assertion could pass with the bug fully
// present and would prove nothing. (Why editor builds survived while packaged ones died is still
// unexplained; see CkSignal_TeardownUafRepro.spec.cpp. Do not encode a theory here.)

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Registry/CkRegistry_Teardown.h"
#include "CkEcs/World/CkEcsWorld.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace ck_registry_teardown_spec
{
    // Stands in for TFragment_Signal_Delegate: records what the window said when its destructor
    // ran, which is exactly the decision the real destructor makes.
    inline bool GDestroyed        = false;
    inline bool GSawWindowOnDtor  = false;

    struct FFragment_Spec_TeardownProbe
    {
        // ReSharper disable once CppInconsistentNaming
        static constexpr auto in_place_delete = true;

        // Carries a payload on purpose: an EMPTY struct is classified as a TAG by the registry
        // (std::is_empty_v) and must then derive from ck::TTag. This stands in for a signal-delegate
        // fragment, which is a real fragment, so it needs to be one too.
        int32 _Unused = 0;

        ~FFragment_Spec_TeardownProbe()
        {
            GDestroyed       = true;
            GSawWindowOnDtor = ck::registry_teardown::Get_IsInProgress();
        }
    };

    static auto Reset() -> void
    {
        GDestroyed       = false;
        GSawWindowOnDtor = false;
    }
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Registry_TeardownWindow_Scope,
    "Ck.CkEcs.Registry.TeardownWindowScope",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Registry_TeardownWindow_Scope::RunTest(const FString&)
{
    using FProbe = ck_registry_teardown_spec::FFragment_Spec_TeardownProbe;

    TestFalse(TEXT("window is closed before anything runs"), ck::registry_teardown::Get_IsInProgress());

    // (1) Ordinary fragment removal — nothing is being torn down, so the window must be CLOSED and
    //     the real destructor must still release its connection.
    {
        ck_registry_teardown_spec::Reset();

        auto World  = ck::FEcsWorld{};
        auto Handle = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(World.Get_Registry());
        Handle.Add<FProbe>();
        Handle.Remove<FProbe>();

        TestTrue(TEXT("probe destructor ran on ordinary removal"),
            ck_registry_teardown_spec::GDestroyed);
        TestFalse(TEXT("ordinary removal does NOT see the teardown window"),
            ck_registry_teardown_spec::GSawWindowOnDtor);
    }

    // (2) Whole-registry destruction — the destructor must see the window OPEN, because the sigh it
    //     would otherwise reach into is being destroyed in the same operation.
    {
        ck_registry_teardown_spec::Reset();

        {
            auto World  = ck::FEcsWorld{};
            auto Handle = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(World.Get_Registry());
            Handle.Add<FProbe>();

            TestFalse(TEXT("window still closed while the world is alive"),
                ck::registry_teardown::Get_IsInProgress());
        }

        TestTrue(TEXT("probe destructor ran during world teardown"),
            ck_registry_teardown_spec::GDestroyed);
        TestTrue(TEXT("teardown-time destructor SEES the window (this is what suppresses the UAF)"),
            ck_registry_teardown_spec::GSawWindowOnDtor);
    }

    TestFalse(TEXT("window is closed again once teardown has finished"),
        ck::registry_teardown::Get_IsInProgress());

    return true;
}

#endif
