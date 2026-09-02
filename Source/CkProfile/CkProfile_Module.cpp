#include "CkProfile_Module.h"

#include "CkProfile/Stats/CkProfile_Stats.h"
#include "CkProfile/Stats/CkScopedStat.h"

#include <Stats/Stats.h>

#if WITH_ANGELSCRIPT_CK
#include <AngelscriptCodeModule.h>
#endif

DECLARE_CYCLE_STAT(TEXT("Script scopes"), STAT_CkScriptScopes, STATGROUP_CkScript);

#define LOCTEXT_NAMESPACE "FCkProfileModule"

#if WITH_ANGELSCRIPT_CK
namespace ck_profile_module
{
    FDelegateHandle GScopeCacheInvalidationHandle;
}
#endif

void FCkProfileModule::StartupModule()
{
#if STATS
    (void)GET_STATID(STAT_CkScriptScopes);
#endif

#if WITH_ANGELSCRIPT_CK
    // The per-function scope cache in CkScopedStat is keyed by asIScriptFunction*, and those
    // pointers neither survive a recompile nor stay unique across one - the allocator recycles the
    // addresses. Without this, a reload would leave entries attributing one function's time to
    // another function's name.
    //
    // NOT editor-gated, deliberately. Script hot reload runs in NON-editor builds too:
    // AngelscriptManager sets bUseHotReloadCheckerThread = bScriptDevelopmentMode && !GIsEditor,
    // so a -game or packaged build launched with -as-development-mode reloads on a dedicated
    // thread - and that is precisely the configuration an editor-only subscription would leave
    // uninvalidated. The PreCompile broadcast itself is unconditional in every configuration.
    // In a build that genuinely never reloads this costs one delegate registration that never fires.
    ck_profile_module::GScopeCacheInvalidationHandle = FAngelscriptCodeModule::GetPreCompile().AddLambda([]
    {
        ck::Invalidate_ScopedStat_ScopeCache();
    });
#endif
}

void FCkProfileModule::ShutdownModule()
{
#if WITH_ANGELSCRIPT_CK
    if (ck_profile_module::GScopeCacheInvalidationHandle.IsValid())
    { FAngelscriptCodeModule::GetPreCompile().Remove(ck_profile_module::GScopeCacheInvalidationHandle); }

    ck_profile_module::GScopeCacheInvalidationHandle.Reset();
#endif
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkProfileModule, CkProfile)
