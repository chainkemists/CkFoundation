#include "CkNavigation/NavSurface/CkNavSurface_ProviderTable.h"

#include "CkNavigation/CkNavigation_Log.h"
#include "CkNavigation/NavSurface/CkNavSurface_Fragment.h"
#include "CkNavigation/Settings/CkNav_ProjectSettings.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"


#include <Engine/World.h>
#include <HAL/CriticalSection.h>
#include <Misc/ScopeLock.h>
#include <Misc/ScopeRWLock.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_nav_surface_provider_table
{
    // CODE, not per-world state: one table per provider for the whole process. A world picks WHICH of
    // them answers it; none of them holds anything a world owns.
    auto Get_Tables() -> TMap<ECk_NavSurface_Provider, FCk_NavSurface_ProviderTable>&
    {
        static auto Tables = TMap<ECk_NavSurface_Provider, FCk_NavSurface_ProviderTable>{};
        return Tables;
    }

    // Registration only. Reads take nothing, because reads happen strictly after every registration
    // has landed (see TryGet_ProviderTable's contract).
    auto Get_RegistrationLock() -> FCriticalSection&
    {
        static auto RegistrationLock = FCriticalSection{};
        return RegistrationLock;
    }

    // Per-world, keyed by the world and nothing else, so a query on any thread can learn which
    // provider a world chose without touching that world's registry.
    auto Get_WorldProviders() -> TMap<TWeakObjectPtr<UWorld>, ECk_NavSurface_Provider>&
    {
        static auto WorldProviders = TMap<TWeakObjectPtr<UWorld>, ECk_NavSurface_Provider>{};
        return WorldProviders;
    }

    auto Get_WorldProvidersLock() -> FRWLock&
    {
        static auto WorldProvidersLock = FRWLock{};
        return WorldProvidersLock;
    }

    auto Get_WorldShadowModes() -> TMap<TWeakObjectPtr<UWorld>, ECk_NavSurface_ShadowMode>&
    {
        static auto WorldShadowModes = TMap<TWeakObjectPtr<UWorld>, ECk_NavSurface_ShadowMode>{};
        return WorldShadowModes;
    }

    auto Get_WorldShadowModesLock() -> FRWLock&
    {
        static auto WorldShadowModesLock = FRWLock{};
        return WorldShadowModesLock;
    }

    auto DoBind_WorldCleanupOnce() -> void
    {
        static auto IsBound = false;

        if (IsBound)
        { return; }

        IsBound = true;

        FWorldDelegates::OnWorldCleanup.AddLambda([](UWorld* InWorld, bool, bool) -> void
        {
            {
                auto Lock = FRWScopeLock{Get_WorldProvidersLock(), SLT_Write};
                Get_WorldProviders().Remove(TWeakObjectPtr<UWorld>{InWorld});
            }

            {
                auto Lock = FRWScopeLock{Get_WorldShadowModesLock(), SLT_Write};
                Get_WorldShadowModes().Remove(TWeakObjectPtr<UWorld>{InWorld});
            }
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_NavSurface_ProviderTable::
    Get_IsComplete() const
    -> bool
{
    return static_cast<bool>(_ProjectPoint) &&
           static_cast<bool>(_MoveAlongSurface) &&
           static_cast<bool>(_SurfaceRaycast) &&
           static_cast<bool>(_BoundarySegments) &&
           static_cast<bool>(_IsReachable) &&
           static_cast<bool>(_SurfaceBounds) &&
           static_cast<bool>(_ProviderHealth) &&
           static_cast<bool>(_IsBuildInProgress) &&
           static_cast<bool>(_IsSurfaceSettled) &&
           static_cast<bool>(_SurfaceRevision) &&
           static_cast<bool>(_RequestSurfaceRebuild) &&
           static_cast<bool>(_ApplyAreaMarkup) &&
           static_cast<bool>(_IsMarkupLive) &&
           static_cast<bool>(_ReleaseAreaMarkup);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::nav_surface::
    Register_Provider(
        ECk_NavSurface_Provider      InProvider,
        FCk_NavSurface_ProviderTable InTable)
    -> void
{
    const auto TableIsComplete = InTable.Get_IsComplete();

    CK_ENSURE_IF_NOT(TableIsComplete,
        TEXT("NavSurface provider [{}] was registered with an INCOMPLETE capability table. The registration is refused."),
        InProvider)
    { return; }

    auto Lock = FScopeLock{&ck_nav_surface_provider_table::Get_RegistrationLock()};

    auto& Tables = ck_nav_surface_provider_table::Get_Tables();

    if (Tables.Contains(InProvider))
    {
        ck::nav::Display
        (
            TEXT("NavSurface provider [{}] was already registered. The previous capability table is REPLACED."),
            InProvider
        );
    }

    Tables.Emplace(InProvider, MoveTemp(InTable));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::nav_surface::
    TryGet_ProviderTable(
        ECk_NavSurface_Provider InProvider)
    -> const FCk_NavSurface_ProviderTable*
{
    return ck_nav_surface_provider_table::Get_Tables().Find(InProvider);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::nav_surface::
    Get_ProviderForWorld(
        UWorld* InWorld)
    -> ECk_NavSurface_Provider
{
    if (InWorld == nullptr)
    { return Get_DefaultProvider(); }

    auto Lock = FRWScopeLock{ck_nav_surface_provider_table::Get_WorldProvidersLock(), SLT_ReadOnly};

    const auto* Chosen = ck_nav_surface_provider_table::Get_WorldProviders().Find(TWeakObjectPtr<UWorld>{InWorld});

    return Chosen != nullptr ? *Chosen : Get_DefaultProvider();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::nav_surface::
    Set_ProviderForWorld(
        UWorld*                 InWorld,
        ECk_NavSurface_Provider InProvider)
    -> void
{
    if (ck::Is_NOT_Valid(InWorld))
    { return; }

    ck_nav_surface_provider_table::DoBind_WorldCleanupOnce();

    auto Lock = FRWScopeLock{ck_nav_surface_provider_table::Get_WorldProvidersLock(), SLT_Write};

    ck_nav_surface_provider_table::Get_WorldProviders().Add(TWeakObjectPtr<UWorld>{InWorld}, InProvider);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::nav_surface::
    Get_ShadowModeForWorld(
        UWorld* InWorld)
    -> ECk_NavSurface_ShadowMode
{
    if (InWorld == nullptr)
    { return Get_DefaultShadowMode(); }

    auto Lock = FRWScopeLock{ck_nav_surface_provider_table::Get_WorldShadowModesLock(), SLT_ReadOnly};

    const auto* Chosen = ck_nav_surface_provider_table::Get_WorldShadowModes().Find(TWeakObjectPtr<UWorld>{InWorld});

    return Chosen != nullptr ? *Chosen : Get_DefaultShadowMode();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::nav_surface::
    Set_ShadowModeForWorld(
        UWorld*                   InWorld,
        ECk_NavSurface_ShadowMode InMode)
    -> void
{
    if (ck::Is_NOT_Valid(InWorld))
    { return; }

    ck_nav_surface_provider_table::DoBind_WorldCleanupOnce();

    auto Lock = FRWScopeLock{ck_nav_surface_provider_table::Get_WorldShadowModesLock(), SLT_Write};

    ck_nav_surface_provider_table::Get_WorldShadowModes().Add(TWeakObjectPtr<UWorld>{InWorld}, InMode);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::nav_surface::
    Get_DefaultProvider()
    -> ECk_NavSurface_Provider
{
    return UCk_Utils_Nav_Settings_UE::Get_DefaultNavSurfaceProvider();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::nav_surface::
    Get_DefaultShadowMode()
    -> ECk_NavSurface_ShadowMode
{
    return UCk_Utils_Nav_Settings_UE::Get_DefaultNavSurfaceShadowMode();
}

// --------------------------------------------------------------------------------------------------------------------
