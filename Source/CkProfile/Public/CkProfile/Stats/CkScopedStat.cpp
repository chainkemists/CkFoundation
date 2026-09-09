#include "CkScopedStat.h"

#include "CkProfile/Stats/CkProfile_Stats.h"
#include "CkProfile/Stats/CkStats.h" // CK_CREATE_DYNAMIC_STAT_ID

#include <Containers/Map.h>
#include <HAL/PlatformMisc.h>
#include <HAL/PlatformTime.h>

#include <atomic>

#if WITH_ANGELSCRIPT_CK
#include "AngelscriptBinds.h"
#include <AngelscriptManager.h>
#include <as_context.h>
#endif

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    namespace
    {
        // The pre-compile callback can run on a worker in packaged builds. It must not clear a
        // different thread's TLS map; lookup observes this epoch and clears its own map instead.
        std::atomic<uint64> GActiveScriptScopeStatCacheEpoch{1};

#if WITH_ANGELSCRIPT_CK
#if STATS
        struct FActiveScriptScopeStatThreadCache
        {
            uint64 _Epoch = 0;
            TMap<int32, TStatId> _StatIds;

#if WITH_DEV_AUTOMATION_TESTS
            uint64 _HitCount = 0;
            uint64 _MissCount = 0;
#endif
        };

        static thread_local FActiveScriptScopeStatThreadCache GActiveScriptScopeStatThreadCache;
#endif

        // The named-event twin of the cache above. BeginNamedEvent takes a NAME rather than a stat
        // id, so the STATS cache cannot serve it - and the derivation it skips is the same one: a
        // script-context walk plus three FString allocations, on every scope entry.
        //
        // Compiled in EVERY configuration on purpose, even though only the non-STATS ctor calls it.
        // Gating it on !STATS would mean the editor never compiles it and no test in any
        // configuration could reach it, which is how the STATS twin came to have this gap.
        struct FActiveScriptScopeNameThreadCache
        {
            uint64               _Epoch = 0;
            TMap<int32, FString> _Names;

#if WITH_DEV_AUTOMATION_TESTS
            uint64 _HitCount = 0;
            uint64 _MissCount = 0;
#endif
        };

        static thread_local FActiveScriptScopeNameThreadCache GActiveScriptScopeNameThreadCache;

        auto
            Get_ScriptScopeName(
                asIScriptFunction* InFunction)
            -> FString
        {
            auto Method = FString{StringCast<TCHAR>(InFunction->GetName()).Get()};
            Method.RemoveFromEnd(TEXT("_Implementation"));

            if (auto* ObjType = InFunction->GetObjectType(); ObjType != nullptr)
            {
                const auto ClassName = FString{StringCast<TCHAR>(ObjType->GetName()).Get()};
                return ClassName + TEXT("::") + Method;
            }

            return Method;
        }
#endif
    }

    auto
        Get_ScopedStat_StatId(
            const FString& InName)
        -> TStatId
    {
#if STATS
        // thread_local keeps this lock-free if scripts ever leave the game thread; the cost is a
        // few duplicated entries per worker thread.
        static thread_local TMap<FName, TStatId> StatIdCache;

        const auto NameKey = FName{InName};
        if (const auto* const Found = StatIdCache.Find(NameKey))
        { return *Found; }

        const auto StatId = CK_CREATE_DYNAMIC_STAT_ID(STATGROUP_CkScript, InName);
        StatIdCache.Add(NameKey, StatId);
        return StatId;
#else
        return TStatId{};
#endif
    }

    auto
        Get_ActiveScriptScopeName()
        -> FString
    {
#if WITH_ANGELSCRIPT_CK
        // GetCurrentScriptContext() is the exported wrapper over asGetActiveContext(). Frame 0 is the
        // script function currently executing — a registered C++ behaviour pushes no script frame.
        auto* Context = FAngelscriptManager::GetCurrentScriptContext();
        if (Context == nullptr)
        { return FString{TEXT("Script::Unknown")}; }

        auto* Func = Context->GetFunction(0);
        if (Func == nullptr)
        { return FString{TEXT("Script::Unknown")}; }

        return Get_ScriptScopeName(Func);
#else
        return FString{TEXT("Script::Unknown")};
#endif
    }

#if STATS
    auto
        Get_ActiveScriptScopeStatId()
        -> TStatId
    {
#if WITH_ANGELSCRIPT_CK
        auto* Context = FAngelscriptManager::GetCurrentScriptContext();
        if (Context == nullptr)
        { return Get_ScopedStat_StatId(FString{TEXT("Script::Unknown")}); }

        auto* Func = Context->GetFunction(0);
        if (Func == nullptr || Func->GetId() < 0)
        { return Get_ScopedStat_StatId(FString{TEXT("Script::Unknown")}); }

        auto& Cache = GActiveScriptScopeStatThreadCache;

        const auto Epoch = GActiveScriptScopeStatCacheEpoch.load(std::memory_order_acquire);
        if (Cache._Epoch != Epoch)
        {
            Cache._StatIds.Reset();
            Cache._Epoch = Epoch;
        }

        const auto FunctionId = Func->GetId();
        if (const auto* const Found = Cache._StatIds.Find(FunctionId))
        {
#if WITH_DEV_AUTOMATION_TESTS
            ++Cache._HitCount;
#endif
            return *Found;
        }

        const auto StatId = Get_ScopedStat_StatId(Get_ScriptScopeName(Func));
        Cache._StatIds.Add(FunctionId, StatId);
#if WITH_DEV_AUTOMATION_TESTS
        ++Cache._MissCount;
#endif
        return StatId;
#else
        return Get_ScopedStat_StatId(FString{TEXT("Script::Unknown")});
#endif
    }
#endif

    auto
        Get_ActiveScriptScopeName_Cached()
        -> const TCHAR*
    {
#if WITH_ANGELSCRIPT_CK
        auto* Context = FAngelscriptManager::GetCurrentScriptContext();
        if (Context == nullptr)
        { return TEXT("Script::Unknown"); }

        auto* Func = Context->GetFunction(0);
        if (Func == nullptr || Func->GetId() < 0)
        { return TEXT("Script::Unknown"); }

        auto& Cache = GActiveScriptScopeNameThreadCache;

        const auto Epoch = GActiveScriptScopeStatCacheEpoch.load(std::memory_order_acquire);
        if (Cache._Epoch != Epoch)
        {
            Cache._Names.Reset();
            Cache._Epoch = Epoch;
        }

        const auto FunctionId = Func->GetId();
        if (const auto* const Found = Cache._Names.Find(FunctionId))
        {
#if WITH_DEV_AUTOMATION_TESTS
            ++Cache._HitCount;
#endif
            return **Found;
        }

        // Returning a pointer INTO the map is safe here, but only because of the ordering above.
        // FString's characters live in a heap allocation the FString object owns, so relocating the
        // object (sparse-array growth reallocs the element storage) moves the owner and not the
        // buffer - an earlier pointer survives a later insert. What would invalidate one is
        // overwriting the SAME key, which destroys the old FString and frees its buffer; that cannot
        // happen here because this Add only ever follows a Find MISS on this thread's own map, with
        // nothing in between that touches it (Get_ScriptScopeName constructs no scope of its own).
        // The pointer dies at the next epoch change, and every caller hands it straight to
        // BeginNamedEvent - whose sinks all copy the text - rather than retaining it.
#if WITH_DEV_AUTOMATION_TESTS
        ++Cache._MissCount;
#endif
        return *Cache._Names.Add(FunctionId, Get_ScriptScopeName(Func));
#else
        return TEXT("Script::Unknown");
#endif
    }

    auto
        Invalidate_ActiveScriptScopeStatCache()
        -> void
    {
        GActiveScriptScopeStatCacheEpoch.fetch_add(1, std::memory_order_release);
    }

#if WITH_DEV_AUTOMATION_TESTS
    auto
        Get_ActiveScriptScopeStatCacheEpoch_ForTests()
        -> uint64
    {
        return GActiveScriptScopeStatCacheEpoch.load(std::memory_order_acquire);
    }

    auto
        Get_IsScopedStatStatsEnabled_ForTests()
        -> bool
    {
#if STATS
        return true;
#else
        return false;
#endif
    }

    auto
        Get_ActiveScriptScopeStatId_ForTests()
        -> uint64
    {
#if STATS
        return reinterpret_cast<uint64>(Get_ActiveScriptScopeStatId().GetRawPointer());
#else
        return 0;
#endif
    }

    auto
        Get_LegacyActiveScriptScopeStatId_ForTests()
        -> uint64
    {
#if STATS
        return reinterpret_cast<uint64>(Get_ScopedStat_StatId(Get_ActiveScriptScopeName()).GetRawPointer());
#else
        return 0;
#endif
    }

    auto
        Get_ActiveScriptScopeStatName_ForTests()
        -> FString
    {
#if STATS
        return Get_ActiveScriptScopeStatId().GetName().ToString();
#else
        return FString{TEXT("Unsupported: STATS=0")};
#endif
    }

    auto
        Get_LegacyActiveScriptScopeStatName_ForTests()
        -> FString
    {
#if STATS
        return Get_ScopedStat_StatId(Get_ActiveScriptScopeName()).GetName().ToString();
#else
        return FString{TEXT("Unsupported: STATS=0")};
#endif
    }

    auto
        Get_ActiveScriptScopeStatDescription_ForTests()
        -> FString
    {
#if STATS
        return FString{Get_ActiveScriptScopeStatId().GetStatDescriptionWIDE()};
#else
        return FString{TEXT("Unsupported: STATS=0")};
#endif
    }

    auto
        Get_ActiveScriptScopeStatCacheHitCount_ForTests()
        -> uint64
    {
#if STATS
#if WITH_ANGELSCRIPT_CK
        return GActiveScriptScopeStatThreadCache._HitCount;
#else
        return 0;
#endif
#else
        return 0;
#endif
    }

    auto
        Get_ActiveScriptScopeStatCacheMissCount_ForTests()
        -> uint64
    {
#if STATS
#if WITH_ANGELSCRIPT_CK
        return GActiveScriptScopeStatThreadCache._MissCount;
#else
        return 0;
#endif
#else
        return 0;
#endif
    }

    auto
        Reset_ActiveScriptScopeStatCacheCounters_ForTests()
        -> void
    {
#if STATS
#if WITH_ANGELSCRIPT_CK
        GActiveScriptScopeStatThreadCache._HitCount = 0;
        GActiveScriptScopeStatThreadCache._MissCount = 0;
#endif
#endif
    }

    auto
        Get_ActiveScriptScopeNameCached_ForTests()
        -> FString
    {
        return FString{Get_ActiveScriptScopeName_Cached()};
    }

    auto
        Get_ActiveScriptScopeNameCacheHitCount_ForTests()
        -> uint64
    {
#if WITH_ANGELSCRIPT_CK
        return GActiveScriptScopeNameThreadCache._HitCount;
#else
        return 0;
#endif
    }

    auto
        Get_ActiveScriptScopeNameCacheMissCount_ForTests()
        -> uint64
    {
#if WITH_ANGELSCRIPT_CK
        return GActiveScriptScopeNameThreadCache._MissCount;
#else
        return 0;
#endif
    }

    auto
        Reset_ActiveScriptScopeNameCacheCounters_ForTests()
        -> void
    {
#if WITH_ANGELSCRIPT_CK
        GActiveScriptScopeNameThreadCache._HitCount = 0;
        GActiveScriptScopeNameThreadCache._MissCount = 0;
#endif
    }

    auto
        Run_ActiveScriptScopeStatBenchmark_ForTests()
        -> FString
    {
#if STATS
        constexpr auto Iterations = 8192;
        constexpr auto Repeats = 3;
        auto CachedTotalSeconds = 0.0;
        auto LegacyTotalSeconds = 0.0;
        auto CachedChecksum = int64{0};
        auto LegacyChecksum = int64{0};

        const auto RunCached = [&CachedChecksum]() -> double
        {
            const auto StartSeconds = FPlatformTime::Seconds();
            for (auto Index = 0; Index < Iterations; ++Index)
            {
                auto Scope = FCk_ScopedStat{};
                CachedChecksum += Index;
            }
            return FPlatformTime::Seconds() - StartSeconds;
        };
        const auto RunLegacy = [&LegacyChecksum]() -> double
        {
            const auto StartSeconds = FPlatformTime::Seconds();
            for (auto Index = 0; Index < Iterations; ++Index)
            {
                const auto ScopeName = Get_ActiveScriptScopeName();
                auto Scope = FCk_ScopedStat{ScopeName};
                LegacyChecksum += Index;
            }
            return FPlatformTime::Seconds() - StartSeconds;
        };

        auto Report = FString{};
        for (auto Repeat = 0; Repeat < Repeats; ++Repeat)
        {
            const auto CachedFirst = (Repeat % 2) == 0;
            const auto CachedSeconds = CachedFirst ? RunCached() : 0.0;
            const auto LegacySeconds = CachedFirst ? RunLegacy() : 0.0;
            const auto LegacySecondsSecond = CachedFirst ? 0.0 : RunLegacy();
            const auto CachedSecondsSecond = CachedFirst ? 0.0 : RunCached();
            const auto ActualCachedSeconds = CachedFirst ? CachedSeconds : CachedSecondsSecond;
            const auto ActualLegacySeconds = CachedFirst ? LegacySeconds : LegacySecondsSecond;
            CachedTotalSeconds += ActualCachedSeconds;
            LegacyTotalSeconds += ActualLegacySeconds;
            Report += FString::Printf(TEXT(" repeat=%d cached=%.3fms legacy=%.3fms"),
                Repeat + 1, ActualCachedSeconds * 1000.0, ActualLegacySeconds * 1000.0);
        }

        return FString::Printf(
            TEXT("[CkProfile ScopedStat lookup bench] actual AS context; iterations=%d repeats=%d cachedMean=%.3fms legacyMean=%.3fms cachedChecksum=%lld legacyChecksum=%lld;%s"),
            Iterations, Repeats, CachedTotalSeconds * 1000.0 / Repeats, LegacyTotalSeconds * 1000.0 / Repeats,
            CachedChecksum, LegacyChecksum, *Report);
#else
        return FString{TEXT("[CkProfile ScopedStat lookup bench] unsupported: STATS=0")};
#endif
    }
#endif // WITH_DEV_AUTOMATION_TESTS
}

// --------------------------------------------------------------------------------------------------------------------

#if STATS

FCk_ScopedStat::
    FCk_ScopedStat()
    : _Cycle(ck::Get_ActiveScriptScopeStatId())
{
}

FCk_ScopedStat::
    FCk_ScopedStat(
        const FString& InName)
    : _Cycle(ck::Get_ScopedStat_StatId(InName))
{
}

FCk_ScopedStat::
    ~FCk_ScopedStat() = default;

#else

// ENABLE_GENERIC_NAMED_EVENTS is 0 in Shipping, where BeginNamedEvent is an empty inline. Without
// this guard the scope would still derive its name for a call that compiles to nothing - caching
// that derivation makes it cheaper, only skipping it makes it free. Begin and End are guarded
// together so they stay paired.
FCk_ScopedStat::
    FCk_ScopedStat()
{
#if ENABLE_GENERIC_NAMED_EVENTS
    FPlatformMisc::BeginNamedEvent(FColor::Red, ck::Get_ActiveScriptScopeName_Cached());
#endif
}

FCk_ScopedStat::
    FCk_ScopedStat(
        const FString& InName)
{
#if ENABLE_GENERIC_NAMED_EVENTS
    FPlatformMisc::BeginNamedEvent(FColor::Red, *InName);
#endif
}

FCk_ScopedStat::
    ~FCk_ScopedStat()
{
#if ENABLE_GENERIC_NAMED_EVENTS
    FPlatformMisc::EndNamedEvent();
#endif
}

#endif

// --------------------------------------------------------------------------------------------------------------------

#if WITH_ANGELSCRIPT_CK

// EOrder::Late is mandatory: the ctor signature references FString, which AngelScript registers
// after the early phase (an Early bind fails with "FString is not a data type").
AS_FORCE_LINK const FAngelscriptBinds::FBind Bind_ck_ScopedStat(FAngelscriptBinds::EOrder::Late, []
{
    FAngelscriptBinds::FNamespace Ns(FString(TEXT("ck")));

    auto Bind = FAngelscriptBinds::ValueClass<FCk_ScopedStat>("ScopedStat", FBindFlags());

    Bind.Constructor("void f()", [](FCk_ScopedStat* Address)
    {
        new(Address) FCk_ScopedStat();
    });

    Bind.Constructor("void f(const FString& in InName)",
        [](FCk_ScopedStat* Address, const FString& InName)
    {
        new(Address) FCk_ScopedStat(InName);
    });

    Bind.Destructor("void f()", [](FCk_ScopedStat* Address)
    {
        Address->~FCk_ScopedStat();
    });

    // Also the seam the AutoTest uses to assert "<Class>::<Method>".
    FAngelscriptBinds::BindGlobalFunction("FString Get_ActiveScriptScopeName()",
        []() -> FString { return ck::Get_ActiveScriptScopeName(); });

#if WITH_DEV_AUTOMATION_TESTS
    FAngelscriptBinds::BindGlobalFunction("int64 Get_ActiveScriptScopeStatCacheEpoch_ForTests()",
        []() -> int64 { return static_cast<int64>(ck::Get_ActiveScriptScopeStatCacheEpoch_ForTests()); });
    FAngelscriptBinds::BindGlobalFunction("void Invalidate_ActiveScriptScopeStatCache_ForTests()",
        []() -> void { ck::Invalidate_ActiveScriptScopeStatCache(); });
    FAngelscriptBinds::BindGlobalFunction("bool Get_IsScopedStatStatsEnabled_ForTests()",
        []() -> bool { return ck::Get_IsScopedStatStatsEnabled_ForTests(); });
    FAngelscriptBinds::BindGlobalFunction("int64 Get_ActiveScriptScopeStatId_ForTests()",
        []() -> int64 { return static_cast<int64>(ck::Get_ActiveScriptScopeStatId_ForTests()); });
    FAngelscriptBinds::BindGlobalFunction("int64 Get_LegacyActiveScriptScopeStatId_ForTests()",
        []() -> int64 { return static_cast<int64>(ck::Get_LegacyActiveScriptScopeStatId_ForTests()); });
    FAngelscriptBinds::BindGlobalFunction("FString Get_ActiveScriptScopeStatName_ForTests()",
        []() -> FString { return ck::Get_ActiveScriptScopeStatName_ForTests(); });
    FAngelscriptBinds::BindGlobalFunction("FString Get_LegacyActiveScriptScopeStatName_ForTests()",
        []() -> FString { return ck::Get_LegacyActiveScriptScopeStatName_ForTests(); });
    FAngelscriptBinds::BindGlobalFunction("FString Get_ActiveScriptScopeStatDescription_ForTests()",
        []() -> FString { return ck::Get_ActiveScriptScopeStatDescription_ForTests(); });
    FAngelscriptBinds::BindGlobalFunction("int64 Get_ActiveScriptScopeStatCacheHitCount_ForTests()",
        []() -> int64 { return static_cast<int64>(ck::Get_ActiveScriptScopeStatCacheHitCount_ForTests()); });
    FAngelscriptBinds::BindGlobalFunction("int64 Get_ActiveScriptScopeStatCacheMissCount_ForTests()",
        []() -> int64 { return static_cast<int64>(ck::Get_ActiveScriptScopeStatCacheMissCount_ForTests()); });
    FAngelscriptBinds::BindGlobalFunction("void Reset_ActiveScriptScopeStatCacheCounters_ForTests()",
        []() -> void { ck::Reset_ActiveScriptScopeStatCacheCounters_ForTests(); });

    FAngelscriptBinds::BindGlobalFunction("FString Get_ActiveScriptScopeNameCached_ForTests()",
        []() -> FString { return ck::Get_ActiveScriptScopeNameCached_ForTests(); });

    FAngelscriptBinds::BindGlobalFunction("int64 Get_ActiveScriptScopeNameCacheHitCount_ForTests()",
        []() -> int64 { return static_cast<int64>(ck::Get_ActiveScriptScopeNameCacheHitCount_ForTests()); });

    FAngelscriptBinds::BindGlobalFunction("int64 Get_ActiveScriptScopeNameCacheMissCount_ForTests()",
        []() -> int64 { return static_cast<int64>(ck::Get_ActiveScriptScopeNameCacheMissCount_ForTests()); });

    FAngelscriptBinds::BindGlobalFunction("void Reset_ActiveScriptScopeNameCacheCounters_ForTests()",
        []() -> void { ck::Reset_ActiveScriptScopeNameCacheCounters_ForTests(); });
    FAngelscriptBinds::BindGlobalFunction("FString Run_ActiveScriptScopeStatBenchmark_ForTests()",
        []() -> FString { return ck::Run_ActiveScriptScopeStatBenchmark_ForTests(); });
#endif
});

#endif

// --------------------------------------------------------------------------------------------------------------------
