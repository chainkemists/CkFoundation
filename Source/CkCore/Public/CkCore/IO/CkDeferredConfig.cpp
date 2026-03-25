#include "CkDeferredConfig.h"
#include "CkIO_Utils.h"

#include "CkCore/CkCoreLog.h"

#include <Misc/CoreDelegates.h>

// --------------------------------------------------------------------------------------------------------------------

TArray<TWeakObjectPtr<UCk_DeferredConfig_UE>> UCk_DeferredConfig_UE::_PendingConfigs;

// Register the engine-init callback at static init time.
// CkCore loads before Angelscript modules, so this is guaranteed to be
// registered before __InitDefaults creates any deferred configs.
namespace
{
    struct FDeferredConfigRegistrar
    {
        FDeferredConfigRegistrar()
        {
            FCoreDelegates::OnFEngineLoopInitComplete.AddStatic(
                &UCk_DeferredConfig_UE::ResolveAllPending);
        }
    };

    static FDeferredConfigRegistrar GDeferredConfigRegistrar;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_DeferredConfig_UE::
    PostInitProperties()
    -> void
{
    Super::PostInitProperties();

    if (IsTemplate())
    { return; }

    if (NOT UCk_Utils_IO_UE::IsEngineSafeForBlockingLoads())
    {
        _PendingConfigs.Add(this);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_DeferredConfig_UE::
    EnsureResolved()
    -> void
{
    if (_IsResolved)
    { return; }

    _IsResolved = true;
    ResolveAssets();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_DeferredConfig_UE::
    ResolveAssets_Implementation()
    -> void
{
    // Default no-op. Subclasses override to resolve soft references.
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_DeferredConfig_UE::
    AreAssetsResolved() const
    -> bool
{
    return _IsResolved;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_DeferredConfig_UE::
    ResolveAllPending()
    -> void
{
    for (auto& WeakConfig : _PendingConfigs)
    {
        if (auto Config = WeakConfig.Get();
            ck::IsValid(Config, ck::IsValid_Policy_NullptrOnly{}))
        {
            Config->EnsureResolved();
        }
    }

    _PendingConfigs.Empty();
}

// --------------------------------------------------------------------------------------------------------------------
