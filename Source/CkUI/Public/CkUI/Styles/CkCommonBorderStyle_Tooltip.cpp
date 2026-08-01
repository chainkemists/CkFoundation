#include "CkUI/Styles/CkCommonBorderStyle_Tooltip.h"

#include "Brushes/SlateColorBrush.h"

UCkCommonBorderStyle_Tooltip::UCkCommonBorderStyle_Tooltip()
{
    Background = FSlateColorBrush(FLinearColor(0.10f, 0.11f, 0.12f, 0.97f)); // Darker than panels so it floats above them
}
