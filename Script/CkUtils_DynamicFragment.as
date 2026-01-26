namespace utils_dynamic_fragment
{
    FCk_Handle
    Add_Fragment(FCk_Handle InHandle, const FAngelscriptAnyStructParameter &in InStructData)
    {
        auto _InHandle = InHandle;
        return _InHandle.Add_Fragment(InStructData.InstancedStruct);
    }
}