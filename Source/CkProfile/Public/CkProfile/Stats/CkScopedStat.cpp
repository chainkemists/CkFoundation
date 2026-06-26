#include "CkScopedStat.h"

#include "CkProfile/Stats/CkProfile_Stats.h"
#include "CkProfile/Stats/CkStats.h" // CK_CREATE_DYNAMIC_STAT_ID

#include <Containers/Map.h>
#include <HAL/PlatformMisc.h>

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
        // Per-(thread, name) cache. Scripts run on the game thread today, but
        // thread_local keeps this lock-free and correct if that ever changes —
        // the cost is a few duplicated entries per worker thread, which is
        // negligible against the FName re-registration we are avoiding.
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
        // GetCurrentScriptContext() is the exported wrapper over asGetActiveContext()
        // (the raw free function isn't exported). Frame 0 is the script function
        // currently executing — i.e. the one that constructed the FCk_ScopedStat —
        // because a registered C++ behaviour doesn't push its own script frame.
        auto* Context = FAngelscriptManager::GetCurrentScriptContext();
        if (Context == nullptr)
        { return FString{TEXT("Script::Unknown")}; }

        auto* Func = Context->GetFunction(0);
        if (Func == nullptr)
        { return FString{TEXT("Script::Unknown")}; }

        auto Method = FString{StringCast<TCHAR>(Func->GetName()).Get()};
        // UFUNCTION(BlueprintOverride) handlers compile to UE's "_Implementation"
        // thunk; strip it so the auto-derived name matches the clean handler name.
        Method.RemoveFromEnd(TEXT("_Implementation"));

        if (auto* ObjType = Func->GetObjectType(); ObjType != nullptr)
        {
            const auto ClassName = FString{StringCast<TCHAR>(ObjType->GetName()).Get()};
            return ClassName + TEXT("::") + Method;
        }

        // Global script function (no owning class) — method name alone.
        return Method;
#else
        return FString{TEXT("Script::Unknown")};
#endif
    }
}

// --------------------------------------------------------------------------------------------------------------------

#if STATS

FCk_ScopedStat::
    FCk_ScopedStat()
    : _Cycle(ck::Get_ScopedStat_StatId(ck::Get_ActiveScriptScopeName()))
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
    FPlatformMisc::BeginNamedEvent(FColor::Red, *ck::Get_ActiveScriptScopeName());
}

FCk_ScopedStat::
    FCk_ScopedStat(
        const FString& InName)
{
    // No stats system: still emit a named CPU event so the scope shows on the
    // Unreal Insights timeline in trace-enabled Test builds (matches CK_STAT's
    // non-STATS fallback).
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

// EOrder::Late: the constructor signature references FString, which AngelScript
// only registers after the early phase (an Early bind sees "FString is not a
// data type"). Every FString-param bind in the codebase registers Late.
AS_FORCE_LINK const FAngelscriptBinds::FBind Bind_ck_ScopedStat(FAngelscriptBinds::EOrder::Late, []
{
    // Registered as ck::ScopedStat — `auto _S = ck::ScopedStat();` is the
    // intended script idiom (AngelScript constructs in place, so the non-copyable
    // guard records exactly once on scope exit).
    FAngelscriptBinds::FNamespace Ns(FString(TEXT("ck")));

    auto Bind = FAngelscriptBinds::ValueClass<FCk_ScopedStat>("ScopedStat", FBindFlags());

    // No-arg: auto-named from the calling script function.
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

    // Expose the auto-name derivation as ck::Get_ActiveScriptScopeName(): useful
    // for diagnostics, and the seam the AutoTest uses to assert "<Class>::<Method>".
    FAngelscriptBinds::BindGlobalFunction("FString Get_ActiveScriptScopeName()",
        []() -> FString { return ck::Get_ActiveScriptScopeName(); });
});

#endif

// --------------------------------------------------------------------------------------------------------------------
