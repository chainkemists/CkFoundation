#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkInput/CkInputSource_Fragment_Data.h"

#include <Subsystems/LocalPlayerSubsystem.h>
#include <UserSettings/EnhancedInputUserSettings.h>

#include "CkInputSource_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * Owns the one input-source entity that belongs to this local player, and is the only thing that maps a
 * ULocalPlayer onto an ECS entity — device ownership and event routing all address the source by handle
 * from there.
 *
 * The entity is created once a PlayerController exists, never at Initialize: the subsystem collection comes
 * up before the engine has given the local player a controller (or a world to create an entity into).
 *
 * It is also the seam that turns a key rebind into a button-map re-derive. The subsystem already owns the
 * handle a derivation request has to land on, and it is the raw-input half of the module that reads user
 * settings here — the keybinding half stays unaware that entities exist.
 */
UCLASS(NotBlueprintable)
class CKINPUT_API UCk_InputSource_Subsystem : public ULocalPlayerSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_InputSource_Subsystem);

public:
    // --- USubsystem ---
    virtual void Deinitialize() override;

    // --- ULocalPlayerSubsystem ---
    virtual void PlayerControllerChanged(APlayerController* NewPlayerController) override;

public:
    /**
     * This local player's input source, creating it if the PlayerController has arrived since the last
     * attempt. Returns an invalid handle while the local player still has no controller.
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|InputSource",
              DisplayName = "[Ck][InputSource] Get Local Player Input Source")
    FCk_Handle_InputSource
    Get_InputSource();

private:
    UFUNCTION()
    void OnSettingsChanged(UEnhancedInputUserSettings* InSettings);

    auto
    DoCreateInputSource() -> void;

    auto
    DoBindToSettings() -> void;

private:
    FCk_Handle_InputSource _InputSource;
    bool _BoundToSettings = false;
};

// --------------------------------------------------------------------------------------------------------------------
