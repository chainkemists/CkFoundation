namespace utils_time
{
    UFUNCTION(
        BlueprintPure,
        DisplayName = "[Ck] Get Time Now",
        Category = "Ck|Utils|Time",
        meta = (DefaultToSelf = "InWorldContextObject"))
    FCk_Time
    Get_TimeNow(
        const UObject InWorldContextObject,
        ECk_Time_WorldTimeType InTimeType = ECk_Time_WorldTimeType::PausedAndDilatedAndClamped)
    {
        auto WorldTimeParams = FCk_Utils_Time_GetWorldTime_Params();
        WorldTimeParams._Object = InWorldContextObject;
        WorldTimeParams.Set_TimeType(InTimeType);
        return utils_time::Get_WorldTime(WorldTimeParams).Get_WorldTime().Get_Time();
    }

    // Build an FCk_Time from a duration expressed in minutes (mirrors the native
    // FCk_Time(seconds) make, for durations that read naturally as minutes).
    FCk_Time FromMinutes(float64 InMinutes)
    {
        return FCk_Time(InMinutes * 60.0f);
    }
}

// Minutes accessor mirroring the native Get_Seconds()/Get_Milliseconds(). Mixin so
// callers write InTime.Get_Minutes(); must live at module scope (AS mixins do not
// resolve from inside a namespace).
mixin float64 Get_Minutes(const FCk_Time& Self)
{
    return Self.Get_Seconds() / 60.0f;
}