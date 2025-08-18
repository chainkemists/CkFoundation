#pragma once

#include <CoreMinimal.h>
#include <CommonTextBlock.h>

#include "CkCommonEditableTextStyle_Default.generated.h"

UCLASS(meta=(DisplayName="CK Editable TextBox Style"))
class CKUI_API UCkCommonEditableTextStyle_Default : public UCommonTextStyle
{
    GENERATED_BODY()

public:
    UCkCommonEditableTextStyle_Default();

private:
    FEditableTextBoxStyle EditableTextBoxStyle;
};
