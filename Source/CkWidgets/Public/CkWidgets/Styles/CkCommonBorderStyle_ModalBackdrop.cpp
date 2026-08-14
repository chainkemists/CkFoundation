#include "CkWidgets/Styles/CkCommonBorderStyle_ModalBackdrop.h"

#include "Brushes/SlateColorBrush.h"

UCkCommonBorderStyle_ModalBackdrop::UCkCommonBorderStyle_ModalBackdrop()
{
    Background = FSlateColorBrush(FLinearColor(0.0f, 0.0f, 0.0f, 0.60f)); // Dim the scene behind modals
}
