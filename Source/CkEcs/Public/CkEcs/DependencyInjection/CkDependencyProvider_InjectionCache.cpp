#include "CkDependencyProvider_InjectionCache.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "UObject/UnrealType.h"

// --------------------------------------------------------------------------------------------------------------------

TMap<TWeakObjectPtr<UClass>, TArray<FCk_InjectionSite>> FCk_InjectionCache::_Plans;
FCriticalSection                                        FCk_InjectionCache::_PlansLock;

// --------------------------------------------------------------------------------------------------------------------

namespace
{
    constexpr auto CkInjectMetaKey = TEXT("CkInject");

    auto
    ParseScope(
        const FString& InScopeString,
        const UClass* InScriptClass,
        const FProperty* InProperty)
        -> TOptional<ECk_DependencyProvider_Scope>
    {
        // Empty value → default to World. Matches plan §1: "Default scope when
        // Inject has no value is World."
        if (InScopeString.IsEmpty() || InScopeString.Equals(TEXT("World"), ESearchCase::IgnoreCase))
        { return ECk_DependencyProvider_Scope::World; }

        if (InScopeString.Equals(TEXT("GameInstance"), ESearchCase::IgnoreCase))
        { return ECk_DependencyProvider_Scope::GameInstance; }

        // Unknown scope value → ensure and skip. The class still loads, but
        // this site won't participate in DI. Author needs to fix the meta.
        CK_ENSURE_IF_NOT(false,
            TEXT("EntityScript [{}] property [{}] has unknown CkInject scope value [{}]. "
                 "Expected one of: \"World\", \"GameInstance\". Property skipped from DI."),
            InScriptClass, InProperty->GetName(), InScopeString)
        { return {}; }

        return {};
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_InjectionCache::
    GetOrBuild(
        UClass* InScriptClass)
    -> const TArray<FCk_InjectionSite>&
{
    // Empty-class fallback. Stable address required because callers cache the
    // ref across the iteration.
    static const auto EmptyPlan = TArray<FCk_InjectionSite>{};

    if (ck::Is_NOT_Valid(InScriptClass))
    { return EmptyPlan; }

    {
        FScopeLock Lock(&_PlansLock);
        if (auto* Existing = _Plans.Find(InScriptClass))
        { return *Existing; }
    }

    // Build outside the lock — DoBuildPlan walks reflection and may take
    // non-trivial time on large classes. Worst case two callers build the
    // same plan concurrently; the second insert is a no-op.
    auto NewPlan = DoBuildPlan(InScriptClass);

    {
        FScopeLock Lock(&_PlansLock);
        auto& Stored = _Plans.FindOrAdd(InScriptClass);
        if (Stored.IsEmpty())
        { Stored = MoveTemp(NewPlan); }
        return Stored;
    }
}

auto
    FCk_InjectionCache::
    InvalidateAll()
    -> void
{
    FScopeLock Lock(&_PlansLock);
    _Plans.Empty();
}

auto
    FCk_InjectionCache::
    DoBuildPlan_ForTesting(
        UClass* InScriptClass)
    -> TArray<FCk_InjectionSite>
{
    return DoBuildPlan(InScriptClass);
}

auto
    FCk_InjectionCache::
    DoBuildPlan(
        UClass* InScriptClass)
    -> TArray<FCk_InjectionSite>
{
    auto Plan = TArray<FCk_InjectionSite>{};
    if (ck::Is_NOT_Valid(InScriptClass))
    { return Plan; }

    const auto* TypeSafeBase = FCk_Handle_TypeSafe::StaticStruct();

    for (auto* Prop : TFieldRange<FStructProperty>(InScriptClass))
    {
        if (NOT Prop->HasMetaData(CkInjectMetaKey))
        { continue; }

        if (ck::Is_NOT_Valid(Prop->Struct))
        { continue; }

        // Only typed handles (subclasses of FCk_Handle_TypeSafe) participate
        // in DI. The contract IS the typed-handle struct, so injecting into a
        // raw FCk_Handle would have no type signal and is rejected.
        if (NOT Prop->Struct->IsChildOf(TypeSafeBase))
        {
            CK_ENSURE_IF_NOT(false,
                TEXT("EntityScript [{}] property [{}] is marked CkInject but its type [{}] "
                     "does not derive from FCk_Handle_TypeSafe. DI requires a typed handle."),
                InScriptClass, Prop->GetName(), Prop->Struct)
            { continue; }
            continue;
        }

        const auto ScopeStr = Prop->GetMetaData(CkInjectMetaKey);
        const auto Scope    = ParseScope(ScopeStr, InScriptClass, Prop);
        if (NOT Scope.IsSet())
        { continue; }

        Plan.Add(FCk_InjectionSite{
            ._PropertyOnScript = Prop,
            ._HandleType       = Prop->Struct,
            ._Scope            = Scope.GetValue()
        });
    }

    return Plan;
}
