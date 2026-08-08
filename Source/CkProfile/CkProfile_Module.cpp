#include "CkProfile_Module.h"

#include "CkProfile/Stats/CkProfile_Stats.h"

#include <Stats/Stats.h>

DECLARE_CYCLE_STAT(TEXT("Script scopes"), STAT_CkScriptScopes, STATGROUP_CkScript);

#define LOCTEXT_NAMESPACE "FCkProfileModule"

void FCkProfileModule::StartupModule()
{
#if STATS
    (void)GET_STATID(STAT_CkScriptScopes);
#endif
}

void FCkProfileModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkProfileModule, CkProfile)
