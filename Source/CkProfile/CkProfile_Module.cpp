#include "CkProfile_Module.h"

#include "CkProfile/Stats/CkProfile_Stats.h"

#include <Stats/Stats.h>
#include <Misc/CoreDelegates.h>
#include "CkProfile/Stats/CkCpuWork.h"

DECLARE_CYCLE_STAT(TEXT("Script scopes"), STAT_CkScriptScopes, STATGROUP_CkScript);

#define LOCTEXT_NAMESPACE "FCkProfileModule"

void FCkProfileModule::StartupModule()
{
#if STATS
    (void)GET_STATID(STAT_CkScriptScopes);
#endif
    FCoreDelegates::OnBeginFrame.AddRaw(this, &FCkProfileModule::OnBeginFrame);
    FCoreDelegates::OnEndFrame.AddRaw(this, &FCkProfileModule::OnEndFrame);
}

void FCkProfileModule::ShutdownModule()
{
    FCoreDelegates::OnBeginFrame.RemoveAll(this);
    FCoreDelegates::OnEndFrame.RemoveAll(this);
}

void FCkProfileModule::OnBeginFrame()
{
    ck::cpu_work::BeginFrame();
}

void FCkProfileModule::OnEndFrame()
{
    ck::cpu_work::EndFrame();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkProfileModule, CkProfile)
