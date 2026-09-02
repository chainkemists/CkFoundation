#include "CkProfile_Module.h"

#include "CkProfile/Stats/CkProfile_Stats.h"
#include "CkProfile/Stats/CkScopedStat.h"

#include <Stats/Stats.h>

#if WITH_EDITOR && WITH_ANGELSCRIPT_CK
#include <AngelscriptCodeModule.h>
#endif

DECLARE_CYCLE_STAT(TEXT("Script scopes"), STAT_CkScriptScopes, STATGROUP_CkScript);

#define LOCTEXT_NAMESPACE "FCkProfileModule"

#if WITH_EDITOR && WITH_ANGELSCRIPT_CK
namespace
{
    FDelegateHandle GScopeCacheInvalidationHandle;
}
#endif

void FCkProfileModule::StartupModule()
{
#if STATS
    (void)GET_STATID(STAT_CkScriptScopes);
#endif

#if WITH_EDITOR && WITH_ANGELSCRIPT_CK
    // The per-function scope cache in CkScopedStat is keyed by asIScriptFunction*, and those
    // pointers neither survive a recompile nor stay unique across one - the allocator recycles the
    // addresses. Without this, a hot reload would leave entries attributing one function's time to
    // another function's name. Editor-only because a packaged game never reloads its scripts.
    GScopeCacheInvalidationHandle = FAngelscriptCodeModule::GetPreCompile().AddLambda([]
    {
        ck::Invalidate_ScopedStat_ScopeCache();
    });
#endif
}

void FCkProfileModule::ShutdownModule()
{
#if WITH_EDITOR && WITH_ANGELSCRIPT_CK
    if (GScopeCacheInvalidationHandle.IsValid())
    { FAngelscriptCodeModule::GetPreCompile().Remove(GScopeCacheInvalidationHandle); }

    GScopeCacheInvalidationHandle.Reset();
#endif
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkProfileModule, CkProfile)
