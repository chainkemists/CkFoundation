#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkGameSettings/CkGameSettings_Common.h"

#include "CkGameSettings_SpecSupport.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/** Reflected change-delegate receiver for the registry spec — dynamic delegates can only bind UFUNCTIONs. */
UCLASS(NotBlueprintable)
class UCk_GameSettings_SpecListener_UE : public UObject
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_GameSettings_SpecListener_UE);

public:
    UFUNCTION()
    void
    OnSettingChanged(
        FName InKey,
        const FString& InNewValue)
    {
        ++_FireCount;
        _LastKey = InKey;
        _LastValue = InNewValue;
    }

private:
    int32 _FireCount = 0;
    FName _LastKey;
    FString _LastValue;

public:
    CK_PROPERTY_GET(_FireCount);
    CK_PROPERTY_GET(_LastKey);
    CK_PROPERTY_GET(_LastValue);
};

// --------------------------------------------------------------------------------------------------------------------
