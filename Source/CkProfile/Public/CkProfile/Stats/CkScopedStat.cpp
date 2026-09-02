#include "CkScopedStat.h"

#include "CkProfile/Stats/CkProfile_Stats.h"
#include "CkProfile/Stats/CkStats.h" // CK_CREATE_DYNAMIC_STAT_ID

#include <Containers/Map.h>
#include <CoreGlobals.h> // GCycleStatsShouldEmitNamedEvents
#include <HAL/PlatformMisc.h>

#include <atomic>

#if WITH_ANGELSCRIPT_CK
#include "AngelscriptBinds.h"
#include <AngelscriptManager.h>
#include <as_context.h>
#endif

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
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

        auto Method = FString{StringCast<TCHAR>(Func->GetName()).Get()};
        Method.RemoveFromEnd(TEXT("_Implementation"));

        if (auto* ObjType = Func->GetObjectType(); ObjType != nullptr)
        {
            const auto ClassName = FString{StringCast<TCHAR>(ObjType->GetName()).Get()};
            return ClassName + TEXT("::") + Method;
        }

        return Method;
#else
        return FString{TEXT("Script::Unknown")};
#endif
    }

    namespace scoped_stat_cache
    {
        // Bumped on every AngelScript recompile. The cache below is thread_local, so a direct clear
        // would only ever reach the calling thread; a generation counter lets each thread drop its
        // own copy lazily on next use, which is correct no matter which thread recompiled.
        static std::atomic<uint32> Generation{0};
    }

    auto
        Invalidate_ScopedStat_ScopeCache()
        -> void
    {
        scoped_stat_cache::Generation.fetch_add(1, std::memory_order_relaxed);
    }

    auto
        Get_ScopedStat_StatId_ForActiveScope()
        -> TStatId
    {
#if STATS
        // nullptr is a real key, not a miss: it covers both "no script context" (a C++ caller, or
        // a natively-bound function that pushes no script frame) and "context with no frame 0".
        // Both name the scope "Script::Unknown", so both correctly share one entry.
        const void* ScopeKey = nullptr;

#if WITH_ANGELSCRIPT_CK
        if (auto* const Context = FAngelscriptManager::GetCurrentScriptContext();
            Context != nullptr)
        { ScopeKey = Context->GetFunction(0); }
#endif

        // thread_local for the same reason Get_ScopedStat_StatId is: lock-free if scripts ever leave
        // the game thread, at the cost of a few duplicated entries per worker.
        static thread_local TMap<const void*, TStatId> ScopeIdCache;
        static thread_local uint32                    CachedGeneration = 0;

        if (const auto CurrentGeneration = scoped_stat_cache::Generation.load(std::memory_order_relaxed);
            CurrentGeneration != CachedGeneration)
        {
            ScopeIdCache.Reset();
            CachedGeneration = CurrentGeneration;
        }

        if (const auto* const Found = ScopeIdCache.Find(ScopeKey))
        { return *Found; }

        // Miss: pay for the name exactly once per script function.
        const auto StatId = Get_ScopedStat_StatId(Get_ActiveScriptScopeName());
        ScopeIdCache.Add(ScopeKey, StatId);
        return StatId;
#else
        return TStatId{};
#endif
    }
}

// --------------------------------------------------------------------------------------------------------------------

#if STATS

FCk_ScopedStat::
    FCk_ScopedStat()
    : _Cycle(ck::Get_ScopedStat_StatId_ForActiveScope())
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

FCk_ScopedStat::
    FCk_ScopedStat()
{
    // Deriving the scope name costs three FString allocations, and with no profiler attached the
    // result is handed to a no-op and discarded - which is every session a player ever runs. Pay it
    // only when something is listening. Begin/End stay unconditionally paired, so the guard cannot
    // unbalance the event stack.
    if (GCycleStatsShouldEmitNamedEvents > 0)
    { FPlatformMisc::BeginNamedEvent(FColor::Red, *ck::Get_ActiveScriptScopeName()); }
    else
    { FPlatformMisc::BeginNamedEvent(FColor::Red, TEXT("Script")); }
}

FCk_ScopedStat::
    FCk_ScopedStat(
        const FString& InName)
{
    FPlatformMisc::BeginNamedEvent(FColor::Red, *InName);
}

FCk_ScopedStat::
    ~FCk_ScopedStat()
{
    FPlatformMisc::EndNamedEvent();
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
});

#endif

// --------------------------------------------------------------------------------------------------------------------
