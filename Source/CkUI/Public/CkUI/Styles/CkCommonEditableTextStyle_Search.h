#pragma once

#include <CoreMinimal.h>
#include <CommonTextBlock.h>

#include "CkCommonEditableTextStyle_Search.generated.h"

UCLASS(meta=(DisplayName="CK Search Bar TextBox Style"))
class CKUI_API UCkCommonEditableTextStyle_Search : public UCommonTextStyle
{
    GENERATED_BODY()

public:
    UCkCommonEditableTextStyle_Search();

private:
    FEditableTextBoxStyle EditableTextBoxStyle;
};
