namespace utils_handle
{
    void Set_DebugName(FCk_Handle InHandle, FString InDebugName)
    {
        utils_handle::Set_DebugName(InHandle, FName(InDebugName));
    }
}