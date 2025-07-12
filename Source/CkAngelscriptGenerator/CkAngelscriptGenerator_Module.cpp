#include "CkAngelscriptGenerator_Module.h"

#include "CkAngelscriptGenerator/CkAngelscriptWrapperGenerator.h"

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "FCkAngelscriptGeneratorModule"

void FCkAngelscriptGeneratorModule::StartupModule()
{
    // FCkAngelscriptWrapperGenerator::GenerateAllWrappers();
}

void FCkAngelscriptGeneratorModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkAngelscriptGeneratorModule, CkAngelscriptGenerator)

// --------------------------------------------------------------------------------------------------------------------