#pragma once

#include <CoreMinimal.h>
#include <CommonTextBlock.h>

#include "CkCommonEditableTextStyle_Numeric.generated.h"

UCLASS(meta=(DisplayName="CK Numeric Editable TextBox Style"))
class CKUI_API UCkCommonEditableTextStyle_Numeric : public UCommonTextStyle
{
    GENERATED_BODY()

public:
    UCkCommonEditableTextStyle_Numeric();

private:
    FEditableTextBoxStyle EditableTextBoxStyle;
};
