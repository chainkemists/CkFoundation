#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Subsystems/GameInstanceSubsystem.h>
#include <Templates/SharedPointer.h>

#include "CkInputSlate_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class FCk_InputSlate_Preprocessor;

// --------------------------------------------------------------------------------------------------------------------

/**
 * Owns the Slate pre-processor that writes raw device events into this game instance's input sources, for
 * exactly as long as the game instance lives.
 *
 * A Slate pre-processor is application-global while an input source belongs to a local player, so the game
 * instance is the narrowest engine seam that owns one of each: registration and unregistration bracket a
 * single game session, and a second PIE session brings up its own subsystem rather than a second registration
 * against the same one.
 */
UCLASS(NotBlueprintable, DisplayName="CkSubsystem_InputSlate")
class CKINPUT_API UCk_InputSlate_Subsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_InputSlate_Subsystem);

public:
    // --- USubsystem ---
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

private:
    auto
    DoRegisterPreprocessor() -> void;

    auto
    DoUnregisterPreprocessor() -> void;

private:
    TSharedPtr<FCk_InputSlate_Preprocessor> _Preprocessor;
};

// --------------------------------------------------------------------------------------------------------------------
