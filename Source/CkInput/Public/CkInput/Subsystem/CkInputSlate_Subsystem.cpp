#include "CkInputSlate_Subsystem.h"

#include "CkInput/CkInput_Log.h"
#include "CkInput/CkInputSlate_Preprocessor.h"

#include <Framework/Application/SlateApplication.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InputSlate_Subsystem::
    Initialize(
        FSubsystemCollectionBase& Collection)
    -> void
{
    Super::Initialize(Collection);

    DoRegisterPreprocessor();
}

auto
    UCk_InputSlate_Subsystem::
    Deinitialize()
    -> void
{
    DoUnregisterPreprocessor();

    Super::Deinitialize();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InputSlate_Subsystem::
    DoRegisterPreprocessor()
    -> void
{
    if (_Preprocessor.IsValid())
    { return; }

    // Absent on a dedicated server and in any headless run, where there is no input to record in the first
    // place.
    if (NOT FSlateApplication::IsInitialized())
    { return; }

    _Preprocessor = MakeShared<FCk_InputSlate_Preprocessor>(GetGameInstance());

    // Index 0 buys the earliest possible view of the event stream, which is what makes arrival order
    // recoverable. It costs nothing to the pre-processors already sitting there because this one never
    // consumes.
    constexpr auto PreprocessorIndex = 0;
    FSlateApplication::Get().RegisterInputPreProcessor(_Preprocessor, PreprocessorIndex);

    ck::input::Display(TEXT("Registered the InputSlate pre-processor at Slate index [{}]"), PreprocessorIndex);
}

auto
    UCk_InputSlate_Subsystem::
    DoUnregisterPreprocessor()
    -> void
{
    if (NOT _Preprocessor.IsValid())
    { return; }

    if (FSlateApplication::IsInitialized())
    { FSlateApplication::Get().UnregisterInputPreProcessor(_Preprocessor); }

    _Preprocessor.Reset();
}

// --------------------------------------------------------------------------------------------------------------------
