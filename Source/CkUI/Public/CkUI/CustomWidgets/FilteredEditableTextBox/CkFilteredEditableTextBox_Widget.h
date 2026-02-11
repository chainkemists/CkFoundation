#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Components/EditableTextBox.h>
#include <CoreMinimal.h>

#include "CkFilteredEditableTextBox_Widget.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Returns true if the character should be allowed.
// FString because TCHAR is not Blueprint-compatible.
DECLARE_DELEGATE_RetVal_OneParam(
    bool,
    FCk_ValidateCharacterDelegate,
    TCHAR);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FCk_TextFilteredEvent,
    const FString&, InRejectedCharacters);

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKUI_API UCk_FilteredEditableTextBox : public UEditableTextBox
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_FilteredEditableTextBox);

public:
    UCk_FilteredEditableTextBox();

protected:
    // C++ override point. Default implementation delegates to OnValidateCharacter
    // if bound, otherwise allows all characters.
    virtual auto
    IsAllowedCharacter(
        TCHAR InChar) const -> bool;

#if WITH_EDITOR
    auto GetPaletteCategory() -> const FText override;
#endif

private:
    UFUNCTION()
    void HandleTextChanged(const FText& InText);

public:
    // Bind this in Blueprint to define which characters are allowed.
    // If unbound, all characters pass through (unless a C++ subclass overrides IsAllowedCharacter).
    FCk_ValidateCharacterDelegate OnValidateCharacter;

    // Fires when one or more characters were rejected during filtering.
    UPROPERTY(BlueprintAssignable, Category = "Filter", meta = (AllowPrivateAccess))
    FCk_TextFilteredEvent OnTextFiltered;

private:
    bool _IsFiltering = false;
};

// --------------------------------------------------------------------------------------------------------------------