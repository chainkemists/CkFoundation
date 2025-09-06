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
        return utils_time::Get_WorldTime(WorldTimeParams)._WorldTime._Time;
    }
}