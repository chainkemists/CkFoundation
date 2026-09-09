#include "CkProfile_Module.h"

#include "CkProfile/Stats/CkProfile_Stats.h"

#include <Stats/Stats.h>
#include <Misc/CoreDelegates.h>
#include "CkProfile/Stats/CkCpuWork.h"
#include "CkProfile/Stats/CkScopedStat.h"

#if WITH_ANGELSCRIPT_CK
#include <AngelscriptCodeModule.h>
#endif

DECLARE_CYCLE_STAT(TEXT("Script scopes"), STAT_CkScriptScopes, STATGROUP_CkScript);

#define LOCTEXT_NAMESPACE "FCkProfileModule"

void FCkProfileModule::StartupModule()
{
#if STATS
    (void)GET_STATID(STAT_CkScriptScopes);
#endif
    FCoreDelegates::OnBeginFrame.AddRaw(this, &FCkProfileModule::OnBeginFrame);
    FCoreDelegates::OnEndFrame.AddRaw(this, &FCkProfileModule::OnEndFrame);

    // NOT gated on STATS. The named-event cache in CkScopedStat.cpp keys on the same epoch and is
    // compiled in every configuration, so registering only under STATS would leave it with nothing
    // to invalidate it - and AngelScript recycles function ids, so a recompile could then hand a
    // recycled id the previous function's name.
#if WITH_ANGELSCRIPT_CK
    _PreCompileDelegateHandle = FAngelscriptCodeModule::GetPreCompile().AddStatic(
        &ck::Invalidate_ActiveScriptScopeStatCache);
    // PreCompile runs before old modules become unavailable; a later callback could still enter
    // an old script scope. Advance again after a successful swap before new code can reuse ids.
    _PostCompileDelegateHandle = FAngelscriptCodeModule::GetPostCompile().AddStatic(
        &ck::Invalidate_ActiveScriptScopeStatCache);
#endif
}

void FCkProfileModule::ShutdownModule()
{
    FCoreDelegates::OnBeginFrame.RemoveAll(this);
    FCoreDelegates::OnEndFrame.RemoveAll(this);

#if WITH_ANGELSCRIPT_CK
    if (_PreCompileDelegateHandle.IsValid() && FModuleManager::Get().IsModuleLoaded(TEXT("AngelscriptCode")))
    { FAngelscriptCodeModule::GetPreCompile().Remove(_PreCompileDelegateHandle); }
    if (_PostCompileDelegateHandle.IsValid() && FModuleManager::Get().IsModuleLoaded(TEXT("AngelscriptCode")))
    { FAngelscriptCodeModule::GetPostCompile().Remove(_PostCompileDelegateHandle); }
    _PreCompileDelegateHandle.Reset();
    _PostCompileDelegateHandle.Reset();
#endif
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
