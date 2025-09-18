namespace utils_screen_fade
{
    void Request_SimpleFadeToBlack(
        APlayerController InOwningController,
        float32 InDuration,
        FCk_Delegate_OnScreenFadeFinished_Dynamic InOnFinished = FCk_Delegate_OnScreenFadeFinished_Dynamic())
    {
        auto FadeOutParams = FCk_ScreenFade_Params(InDuration);
        FadeOutParams
            .Set_FromColor(FLinearColor::Transparent)
            .Set_ToColor(FLinearColor::Black)
            .Set_OnFinishedDynamic(InOnFinished);

        utils_screen_fade::Request_ScreenFade(InOwningController, FadeOutParams);
    }

    void Request_SimpleFadeFromBlack(
        APlayerController InOwningController,
        float32 InDuration,
        FCk_Delegate_OnScreenFadeFinished_Dynamic InOnFinished = FCk_Delegate_OnScreenFadeFinished_Dynamic())
    {
        auto FadeOutParams = FCk_ScreenFade_Params(InDuration);
        FadeOutParams
            .Set_FromColor(FLinearColor::Black)
            .Set_ToColor(FLinearColor::Transparent)
            .Set_OnFinishedDynamic(InOnFinished);

        utils_screen_fade::Request_ScreenFade(InOwningController, FadeOutParams);
    }
}