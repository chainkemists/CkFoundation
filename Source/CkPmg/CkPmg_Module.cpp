#include "CkPmg_Module.h"
#include "CkPmg/CkPmg_FontGlyphCache.h"

#define LOCTEXT_NAMESPACE "FCkPmgModule"

void FCkPmgModule::StartupModule()
{
}

void FCkPmgModule::ShutdownModule()
{
    ck::pmg::FFontGlyphCache::Get().Shutdown();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkPmgModule, CkPmg)
