namespace utils_vfx
{
    FCk_Handle_PendingEntityScript
    Request_ExecuteCue(
        FCk_Handle InHandle,
        FGameplayTag InTag,
        FAngelscriptAnyStructParameter InSpawnParams)
    {
        auto VfxCueExecutor = Subsystem::GetWorldSubsystem(UCk_VfxCueExecutor_Subsystem_UE);
        return VfxCueExecutor.Request_ExecuteCue(InHandle, InTag, InSpawnParams.InstancedStruct);
    }

    FCk_Handle_PendingEntityScript
    Request_ExecuteCue_Local(
        FCk_Handle InHandle,
        FGameplayTag InTag,
        FAngelscriptAnyStructParameter InSpawnParams)
    {
        auto VfxCueExecutor = Subsystem::GetWorldSubsystem(UCk_VfxCueExecutor_Subsystem_UE);
        return VfxCueExecutor.Request_ExecuteCue_Local(InHandle, InTag, InSpawnParams.InstancedStruct);
    }
}

// Note: Angelscript overload resolution differs from C++.
// Each argument receives a conversion cost, and the overload with the
// lowest maximum cost is selected. Passing FNames created via n"test"
// produces a const FName& which incurs a high-cost copy if the bound
// function takes FName by value. When two overloads both require that
// copy, Angelscript sees their maximum costs as identical and reports
// multiple matching signatures.
// Using const FName& in the bindings avoids that conversion and lets
// the float/int overload resolve correctly while improving performance.
// Note: I don't buy the performance argument for types such as FName
// reference: https://discord.com/channels/551756549962465299/551756549962465303/1237974840753917995
namespace utils_vfx
{
    FCk_VfxCue_UserParameter
    MakeUserParameter(const FName& InName, float InValue)
    {
        auto UserParameter = FCk_VfxCue_UserParameter();
        UserParameter.Set_ParameterName(InName);
        UserParameter.Set_ParameterType(ECk_VfxCue_ParameterType::Float);
        UserParameter.Set_FloatValue(InValue);
        return UserParameter;
    }

    FCk_VfxCue_UserParameter
    MakeUserParameter(const FName& InName, int32 InValue)
    {
        auto UserParameter = FCk_VfxCue_UserParameter();
        UserParameter.Set_ParameterName(InName);
        UserParameter.Set_ParameterType(ECk_VfxCue_ParameterType::Int);
        UserParameter.Set_IntValue(InValue);
        return UserParameter;
    }

    FCk_VfxCue_UserParameter
    MakeUserParameter(const FName& InName, FVector InValue)
    {
        return FCk_VfxCue_UserParameter(InName, InValue);
    }

    FCk_VfxCue_UserParameter
    MakeUserParameter(const FName& InName, FLinearColor InValue)
    {
        return FCk_VfxCue_UserParameter(InName, InValue);
    }

    FCk_VfxCue_UserParameter
    MakeUserParameter(const FName& InName, bool InValue)
    {
        return FCk_VfxCue_UserParameter(InName, InValue);
    }
}