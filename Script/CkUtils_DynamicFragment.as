namespace utils_dynamic_fragment
{
    FInstancedStruct& AddOrGet_Fragment(FCk_Handle InHandle, UScriptStruct InStructType)
    {
        if (utils_dynamic_fragment::Has_Fragment(InHandle, InStructType) == false)
        {
            auto DynamicFragment = FAngelscriptAnyStructParameter();
            DynamicFragment.InstancedStruct.InitializeAs(InStructType);
            utils_dynamic_fragment::Add_Fragment(InHandle, DynamicFragment);
        }

        auto& Fragment = utils_dynamic_fragment::Get_Fragment(InHandle, InStructType);
        return Fragment;
    }
}