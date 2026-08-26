// Cooked-safe Dynamic Fragment display-schema contract tests.

#include "CkDynamic/CkDynamic_FragmentDisplaySchema.h"
#include "CkDynamic/CkDynamic_Fragment_Data.h"
#include "CkDynamic/CkDynamic_Utils.h"

#include "CkCore/Ensure/CkEnsure_Tracker.h"
#include "CkCore/Macros/CkMacros.h"

#include "Async/Async.h"
#include "Containers/Ticker.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

#include <atomic>

#if WITH_ANGELSCRIPT_CK
#include <AngelscriptManager.h>
#endif

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_DynamicFragment_DisplaySchema_ValueOwnership,
    "Ck.CkDynamic.DisplaySchema.ValueOwnershipAndStableKeys",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_DynamicFragment_DisplaySchema_AsReplacement,
    "Ck.CkDynamic.DisplaySchema.AngelScriptReplacementPreservesNative",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_DynamicFragment_DisplaySchema_Resolution,
    "Ck.CkDynamic.DisplaySchema.ExactResolutionAndFallback",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

#if WITH_ANGELSCRIPT_CK
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_DynamicFragment_DisplaySchema_WorkerThreadRefresh,
    "Ck.CkDynamic.DisplaySchema.WorkerThreadRefreshMarshalsToGameThread",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
#endif

// --------------------------------------------------------------------------------------------------------------------

bool FCkTest_DynamicFragment_DisplaySchema_ValueOwnership::RunTest(const FString&)
{
    const auto TypePath = FString{TEXT("/Script/CkDynamic.Tests.DisplaySchema.ValueOwnership")};
    const auto EnumPath = FString{TEXT("/Script/CkDynamic.Tests.DisplaySchema.Enum")};
    auto PreviousSchema = ck::dynamic::FFragmentDisplaySchema{};
    const auto HadPreviousSchema = ck::dynamic::TryGet_NativeFragmentDisplaySchema(TypePath, PreviousSchema);

    auto Schema = ck::dynamic::FFragmentDisplaySchema{};
    Schema.FragmentDisplayName = TEXT("Exact Fragment Label");
    Schema.PropertyDisplayNames.Add(FName{TEXT("RawProperty")}, TEXT("Exact Property Label"));
    auto EnumDisplayNames = TMap<int64, FString>{};
    EnumDisplayNames.Add(17, TEXT("Exact Enum Label"));
    Schema.EnumValueDisplayNames.Add(EnumPath, MoveTemp(EnumDisplayNames));

    TestTrue(TEXT("valid native schema is accepted"),
        ck::dynamic::Register_NativeFragmentDisplaySchema(TypePath, Schema));

    // Mutating the caller's values after registration cannot mutate the registry.
    Schema.FragmentDisplayName = TEXT("Mutated Caller Label");
    Schema.PropertyDisplayNames[FName{TEXT("RawProperty")}] = TEXT("Mutated Caller Property");
    Schema.EnumValueDisplayNames[EnumPath][17] = TEXT("Mutated Caller Enum");

    auto Stored = ck::dynamic::FFragmentDisplaySchema{};
    TestTrue(TEXT("registered schema can be copied out"),
        ck::dynamic::TryGet_FragmentDisplaySchema(TypePath, Stored));
    TestEqual(TEXT("fragment label is value-owned"), Stored.FragmentDisplayName, FString{TEXT("Exact Fragment Label")});
    TestEqual(TEXT("property label uses the stable authored-name key"),
        Stored.PropertyDisplayNames.FindRef(FName{TEXT("RawProperty")}), FString{TEXT("Exact Property Label")});
    TestEqual(TEXT("enum label uses stable path and numeric-value keys"),
        Stored.EnumValueDisplayNames.FindRef(EnumPath).FindRef(17), FString{TEXT("Exact Enum Label")});

    const auto InvalidPath = FString{TEXT("/Script/CkDynamic.Tests.DisplaySchema.Invalid")};
    auto InvalidSchema = ck::dynamic::FFragmentDisplaySchema{};
    InvalidSchema.FragmentDisplayName = TEXT("Would Partially Publish");
    InvalidSchema.PropertyDisplayNames.Add(NAME_None, TEXT("Invalid Key"));

    // AT LEAST once, not exactly once. The rejection is a CK_ENSURE, and how many log lines one ensure
    // produces is a property of the HOST rather than of the code under test: outside PIE the ensure's own
    // diagnostic and the engine's assertion-failure line both carry the message, so the exact-count form reds
    // on a headless lane while passing in the editor. Zero is still a failure — the ensure firing is the
    // assertion, and the two TestFalse checks below are what say it rejected atomically.
    constexpr auto AtLeastOnce = 0;
    AddExpectedError(
        TEXT("Invalid native Dynamic Fragment display schema registration"),
        EAutomationExpectedErrorFlags::Contains,
        AtLeastOnce);
    TestFalse(TEXT("invalid schema is rejected atomically"),
        ck::dynamic::Register_NativeFragmentDisplaySchema(InvalidPath, MoveTemp(InvalidSchema)));
    TestFalse(TEXT("invalid schema publishes no partial entry"),
        ck::dynamic::TryGet_FragmentDisplaySchema(InvalidPath, Stored));

    // Optional diagnostics can never suppress core storage-id admission for a valid fragment type.
    const auto* FragmentType = FCk_Fragment_DynamicFragment_Data::StaticStruct();
    const auto StorageId = UCk_Utils_DynamicFragment_UE::Get_StorageId(FragmentType);
    TestEqual(TEXT("display-schema rejection does not alter storage ids"),
        StorageId, entt::id_type{GetTypeHash(FragmentType->GetPathName())});

    TestTrue(TEXT("native fixture is removed"),
        ck::dynamic::Unregister_NativeFragmentDisplaySchema(TypePath));
    if (HadPreviousSchema)
    {
        TestTrue(TEXT("prior native fixture path is restored"),
            ck::dynamic::Register_NativeFragmentDisplaySchema(TypePath, MoveTemp(PreviousSchema)));
    }
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

bool FCkTest_DynamicFragment_DisplaySchema_AsReplacement::RunTest(const FString&)
{
    const auto NativePath = FString{TEXT("/Script/CkDynamic.Tests.DisplaySchema.PersistentNative")};
    const auto OldAsPath = FString{TEXT("/Script/Angelscript.Tests.DisplaySchema.Old")};
    const auto NewAsPath = FString{TEXT("/Script/Angelscript.Tests.DisplaySchema.New")};
    const auto PreviousAsSchemas = ck::dynamic::Get_AngelscriptFragmentDisplaySchemas();
    auto PreviousNativeSchema = ck::dynamic::FFragmentDisplaySchema{};
    const auto HadPreviousNativeSchema = ck::dynamic::TryGet_NativeFragmentDisplaySchema(
        NativePath, PreviousNativeSchema);

    auto NativeSchema = ck::dynamic::FFragmentDisplaySchema{};
    NativeSchema.FragmentDisplayName = TEXT("Native Label");
    TestTrue(TEXT("native baseline is registered"),
        ck::dynamic::Register_NativeFragmentDisplaySchema(NativePath, MoveTemp(NativeSchema)));

    auto OldAsSchema = ck::dynamic::FFragmentDisplaySchema{};
    OldAsSchema.FragmentDisplayName = TEXT("Old AS Label");
    auto FirstGeneration = TMap<FString, ck::dynamic::FFragmentDisplaySchema>{};
    FirstGeneration.Add(OldAsPath, MoveTemp(OldAsSchema));
    TestTrue(TEXT("first AS generation is published atomically"),
        ck::dynamic::Replace_AngelscriptFragmentDisplaySchemas(MoveTemp(FirstGeneration)));

    auto NewAsSchema = ck::dynamic::FFragmentDisplaySchema{};
    NewAsSchema.FragmentDisplayName = TEXT("New AS Label");
    auto SecondGeneration = TMap<FString, ck::dynamic::FFragmentDisplaySchema>{};
    SecondGeneration.Add(NewAsPath, MoveTemp(NewAsSchema));
    TestTrue(TEXT("hot-reload generation replacement succeeds"),
        ck::dynamic::Replace_AngelscriptFragmentDisplaySchemas(MoveTemp(SecondGeneration)));

    auto Stored = ck::dynamic::FFragmentDisplaySchema{};
    TestTrue(TEXT("native entry survives AS refresh"),
        ck::dynamic::TryGet_FragmentDisplaySchema(NativePath, Stored));
    TestEqual(TEXT("native entry is unchanged"), Stored.FragmentDisplayName, FString{TEXT("Native Label")});
    TestFalse(TEXT("stale AS generation is removed"),
        ck::dynamic::TryGet_FragmentDisplaySchema(OldAsPath, Stored));
    TestTrue(TEXT("new AS generation is visible"),
        ck::dynamic::TryGet_FragmentDisplaySchema(NewAsPath, Stored));
    TestEqual(TEXT("new AS label is exact"), Stored.FragmentDisplayName, FString{TEXT("New AS Label")});

    TestTrue(TEXT("prior AS registry is restored"),
        ck::dynamic::Replace_AngelscriptFragmentDisplaySchemas(PreviousAsSchemas));
    TestTrue(TEXT("native fixture is removed"),
        ck::dynamic::Unregister_NativeFragmentDisplaySchema(NativePath));
    if (HadPreviousNativeSchema)
    {
        TestTrue(TEXT("prior native fixture path is restored"),
            ck::dynamic::Register_NativeFragmentDisplaySchema(NativePath, MoveTemp(PreviousNativeSchema)));
    }
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

bool FCkTest_DynamicFragment_DisplaySchema_Resolution::RunTest(const FString&)
{
    const auto* FragmentType = FCk_DynamicFragment_RepNotifyInfo::StaticStruct();
    const auto* Property = FindFProperty<FProperty>(FragmentType, GET_MEMBER_NAME_CHECKED(FCk_DynamicFragment_RepNotifyInfo, ChangedType));
    const auto* Enum = StaticEnum<ECk_DestroyFilter>();
    auto PreviousSchema = ck::dynamic::FFragmentDisplaySchema{};
    const auto HadPreviousSchema = ck::dynamic::TryGet_NativeFragmentDisplaySchema(
        FragmentType->GetPathName(), PreviousSchema);

    if (Property == nullptr || Enum == nullptr)
    {
        AddError(TEXT("Display-schema resolution fixtures are unavailable"));
        return false;
    }

    auto Schema = ck::dynamic::FFragmentDisplaySchema{};
    Schema.FragmentDisplayName = TEXT("Custom Fragment Title");
    Schema.PropertyDisplayNames.Add(Property->GetFName(), TEXT("Custom Property Title"));
    auto EnumDisplayNames = TMap<int64, FString>{};
    EnumDisplayNames.Add(static_cast<int64>(ECk_DestroyFilter::Teardown), TEXT("Custom Enum Choice"));
    Schema.EnumValueDisplayNames.Add(
        Enum->GetPathName(),
        MoveTemp(EnumDisplayNames));
    TestTrue(TEXT("resolver fixture schema is accepted"),
        ck::dynamic::Register_NativeFragmentDisplaySchema(FragmentType->GetPathName(), MoveTemp(Schema)));

    TestEqual(TEXT("custom fragment label resolves exactly"),
        ck::dynamic::Resolve_FragmentDisplayName(FragmentType), FString{TEXT("Custom Fragment Title")});
    TestEqual(TEXT("custom property label resolves exactly"),
        ck::dynamic::Resolve_PropertyDisplayName(FragmentType, Property), FString{TEXT("Custom Property Title")});
    TestEqual(TEXT("custom enum label resolves exactly"),
        ck::dynamic::Resolve_EnumValueDisplayName(
            FragmentType, Enum, static_cast<int64>(ECk_DestroyFilter::Teardown)),
        FString{TEXT("Custom Enum Choice")});

    const auto* FallbackType = FCk_Fragment_DynamicFragment_Data::StaticStruct();
    auto PreviousFallbackSchema = ck::dynamic::FFragmentDisplaySchema{};
    const auto HadPreviousFallbackSchema = ck::dynamic::TryGet_NativeFragmentDisplaySchema(
        FallbackType->GetPathName(), PreviousFallbackSchema);
    if (HadPreviousFallbackSchema)
    { ck::dynamic::Unregister_NativeFragmentDisplaySchema(FallbackType->GetPathName()); }

    TestEqual(TEXT("producer without schema gets deterministic raw-name fallback"),
        ck::dynamic::Resolve_FragmentDisplayName(FallbackType),
        FName::NameToDisplayString(FallbackType->GetName(), false));

    if (HadPreviousFallbackSchema)
    {
        TestTrue(TEXT("prior fallback-type schema is restored"),
            ck::dynamic::Register_NativeFragmentDisplaySchema(
                FallbackType->GetPathName(), MoveTemp(PreviousFallbackSchema)));
    }

    TestTrue(TEXT("real-type fixture is removed"),
        ck::dynamic::Unregister_NativeFragmentDisplaySchema(FragmentType->GetPathName()));
    if (HadPreviousSchema)
    {
        TestTrue(TEXT("prior real-type schema is restored"),
            ck::dynamic::Register_NativeFragmentDisplaySchema(FragmentType->GetPathName(), MoveTemp(PreviousSchema)));
    }
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#if WITH_ANGELSCRIPT_CK

bool FCkTest_DynamicFragment_DisplaySchema_WorkerThreadRefresh::RunTest(const FString&)
{
    // Cooked AngelScript initializes on a worker, so its PostCompile broadcast arrives off the game thread. The
    // refresh's game-thread invariant is what this marshal exists to satisfy - without it the packaged boot ensures
    // and publishes nothing.
    if (NOT FAngelscriptManager::IsInitialized())
    {
        AddInfo(TEXT("AngelScript is not initialized in this host - the marshal cannot be exercised"));
        return true;
    }

    const auto NativePath = FString{TEXT("/Script/CkDynamic.Tests.DisplaySchema.WorkerThreadNative")};
    const auto StaleAsPath = FString{TEXT("/Script/Angelscript.Tests.DisplaySchema.WorkerThreadStale")};
    const auto PreviousAsSchemas = ck::dynamic::Get_AngelscriptFragmentDisplaySchemas();
    auto PreviousNativeSchema = ck::dynamic::FFragmentDisplaySchema{};
    const auto HadPreviousNativeSchema = ck::dynamic::TryGet_NativeFragmentDisplaySchema(
        NativePath, PreviousNativeSchema);

    auto NativeSchema = ck::dynamic::FFragmentDisplaySchema{};
    NativeSchema.FragmentDisplayName = TEXT("Native Survives The Marshal");
    TestTrue(TEXT("native baseline is registered"),
        ck::dynamic::Register_NativeFragmentDisplaySchema(NativePath, MoveTemp(NativeSchema)));

    // A sentinel generation no observed path can produce, so its DISAPPEARANCE is the proof that the refresh ran.
    auto StaleSchema = ck::dynamic::FFragmentDisplaySchema{};
    StaleSchema.FragmentDisplayName = TEXT("Stale AS Generation");
    auto StaleGeneration = TMap<FString, ck::dynamic::FFragmentDisplaySchema>{};
    StaleGeneration.Add(StaleAsPath, MoveTemp(StaleSchema));
    TestTrue(TEXT("stale AS generation is seeded"),
        ck::dynamic::Replace_AngelscriptFragmentDisplaySchemas(MoveTemp(StaleGeneration)));

    const auto EnsureCountBefore = ck::ensure::Get_EnsureOccurrenceTracker().GetTotalCount();

    std::atomic<bool> RequestRanOnWorker = false;
    auto Future = Async(EAsyncExecution::ThreadPool, [&RequestRanOnWorker]()
    {
        RequestRanOnWorker = NOT IsInGameThread();
        ck::dynamic::Request_RefreshAngelscriptFragmentDisplaySchemas();
    });
    Future.Wait();

    TestTrue(TEXT("the request is issued off the game thread"), RequestRanOnWorker.load());

    // The worker leg is the whole point: it may not touch the registry and it may not ensure. Scoped tightly to the
    // Async window because the ticker pump below runs every OTHER pending core ticker too.
    TestEqual(TEXT("the worker leg fires no ensure"),
        ck::ensure::Get_EnsureOccurrenceTracker().GetTotalCount(), EnsureCountBefore);

    auto Stored = ck::dynamic::FFragmentDisplaySchema{};
    TestTrue(TEXT("the worker leg does not publish - the stale generation is still standing"),
        ck::dynamic::TryGet_FragmentDisplaySchema(StaleAsPath, Stored));

    FTSTicker::GetCoreTicker().Tick(0.0f);

    TestFalse(TEXT("the game-thread pump replaced the AS generation"),
        ck::dynamic::TryGet_FragmentDisplaySchema(StaleAsPath, Stored));
    TestTrue(TEXT("native entries survive the marshalled refresh"),
        ck::dynamic::TryGet_FragmentDisplaySchema(NativePath, Stored));
    TestEqual(TEXT("the native entry is unchanged"),
        Stored.FragmentDisplayName, FString{TEXT("Native Survives The Marshal")});

    TestTrue(TEXT("prior AS registry is restored"),
        ck::dynamic::Replace_AngelscriptFragmentDisplaySchemas(PreviousAsSchemas));
    TestTrue(TEXT("native fixture is removed"),
        ck::dynamic::Unregister_NativeFragmentDisplaySchema(NativePath));
    if (HadPreviousNativeSchema)
    {
        TestTrue(TEXT("prior native fixture path is restored"),
            ck::dynamic::Register_NativeFragmentDisplaySchema(NativePath, MoveTemp(PreviousNativeSchema)));
    }
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#endif // WITH_ANGELSCRIPT_CK

#endif
