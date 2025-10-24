#include "CkAudioEditor_Module.h"

#define LOCTEXT_NAMESPACE "FCkAudioEditorModule"

// ----------------------------------------------------------------------------------------------------------------

auto
    FCkAudioEditorModule::
    StartupModule()
    -> void
{
}

auto
    FCkAudioEditorModule::
    ShutdownModule()
    -> void
{
}

// ----------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkAudioEditorModule, CkAudioEditor)