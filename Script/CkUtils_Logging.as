namespace ck
{
    void Trace(FString InText, FName InKey = NAME_None, float InDuration = 2.0f, FLinearColor InColor = FLinearColor::White)
    {
        System::PrintString(InText, true, true, InColor, InDuration, InKey);
    }

    void Warning(FString InText, FName InKey = NAME_None, float InDuration = 2.0f, FLinearColor InColor = FLinearColor::Yellow)
    {
        System::PrintString(InText, true, true, InColor, InDuration, InKey);
    }

    void Error(FString InText, FName InKey = NAME_None, float InDuration = 2.0f, FLinearColor InColor = FLinearColor::Red)
    {
        System::PrintString(InText, true, true, InColor, InDuration, InKey);
    }
}