namespace utils_dynamic_fragment
{
    FInstancedStruct& AddOrGet_Fragment(FCk_Handle InHandle, UScriptStruct InStructType)
    {
        if (utils_dynamic_fragment::Has_Fragment(InHandle, InStructType) == false)
        {
            utils_dynamic_fragment::Add_Fragment(InHandle, InStructType.Instanced());
        }

        auto& Fragment = utils_dynamic_fragment::Get_Fragment(InHandle, InStructType);
        return Fragment;
    }

    FCk_Handle
    Add_Fragment(FCk_Handle InHandle, const FAngelscriptAnyStructParameter &in InStructData)
    {
        auto _InHandle = InHandle;
        return UCk_Utils_DynamicFragment_UE::Add_Fragment(_InHandle, InStructData.InstancedStruct);
    }
}