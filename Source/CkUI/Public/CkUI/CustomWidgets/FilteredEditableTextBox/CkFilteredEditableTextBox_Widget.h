#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Components/EditableTextBox.h>
#include <CoreMinimal.h>

#include "CkFilteredEditableTextBox_Widget.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Returns true if the character should be allowed (C++ only, uses TCHAR).
DECLARE_DELEGATE_RetVal_OneParam(
    bool,
    FCk_ValidateCharacterDelegate,
    TCHAR);

// Blueprint version: receives the character as a single-character FString; return true to allow it.
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(
    bool,
    FCk_ValidateCharacterDelegate_BP,
    const FString&, InCharacter);

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
    // Override point: the default delegates to OnValidateCharacter when bound, otherwise allows everything.
    virtual auto
    IsAllowedCharacter(
        TCHAR InChar) const -> bool;

#if WITH_EDITOR
    auto GetPaletteCategory() -> const FText override;
#endif

private:
    UFUNCTION()
    void HandleTextChanged(const FText& InText);

    // Blueprint version: returns true if a max length is enforced, with the length as an out param.
    UFUNCTION(BlueprintCallable, Category = "Filter", DisplayName = "Get Max Length")
    bool Get_MaxLength_BP(int32& OutMaxLength) const;

public:
    // Bind in C++ to define which characters are allowed; unbound means everything passes.
    FCk_ValidateCharacterDelegate OnValidateCharacter;

    // Blueprint-bindable twin, evaluated after OnValidateCharacter — BOTH must allow a character for it to pass.
    UPROPERTY(BlueprintReadWrite, Category = "Filter", DisplayName = "On Validate Character", meta = (AllowPrivateAccess = true))
    FCk_ValidateCharacterDelegate_BP OnValidateCharacter_BP;

    UPROPERTY(BlueprintAssignable, Category = "Filter", meta = (AllowPrivateAccess = true))
    FCk_TextFilteredEvent OnTextFiltered;

    auto Get_MaxLength() const -> TOptional<int32>;

    UFUNCTION(BlueprintCallable, Category = "Filter", DisplayName = "Set Max Length")
    void Request_SetMaxLength(int32 InMaxLength);

private:
    UPROPERTY(EditAnywhere, Category = "Filter", meta = (AllowPrivateAccess = true))
    bool _EnforceMaxLength = false;

    UPROPERTY(EditAnywhere, Category = "Filter", meta = (AllowPrivateAccess = true, EditCondition = "_EnforceMaxLength", ClampMin = "1"))
    int32 _MaxLength = 32;

    bool _IsFiltering = false;
};

// --------------------------------------------------------------------------------------------------------------------