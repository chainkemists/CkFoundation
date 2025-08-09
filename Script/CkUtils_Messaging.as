namespace utils_messaging
{
    void
    Broadcast(FCk_Handle InHandle, FAngelscriptAnyStructParameter InPayload)
    {
        auto MessagePayloadStructName = InPayload.InstancedStruct.ScriptStruct.GetName();
        auto MessageName = GameplayTags::ResolveGameplayTag(MessagePayloadStructName, "Generated from a Message payload definition");
        utils_messaging::Broadcast(InHandle, MessageName, InPayload.InstancedStruct);
    }

    void
    UnbindFrom_OnBroadcast(FCk_Handle InHandle, UScriptStruct MessagePayloadTemplate, const FCk_Delegate_Messaging_OnBroadcast &in InDelegate)
    {
        auto MessagePayloadStructName = MessagePayloadTemplate.GetName();
        auto MessageName = GameplayTags::ResolveGameplayTag(MessagePayloadStructName, "Generated from a Message payload definition");
        utils_messaging::UnbindFrom_OnBroadcast(InHandle, MessageName, InDelegate);
    }

    void
    BindTo_OnBroadcast(
        FCk_Handle InHandle,
        UScriptStruct MessagePayloadTemplate, 
        const FCk_Delegate_Messaging_OnBroadcast &in InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing)
    {
        auto MessagePayloadStructName = MessagePayloadTemplate.GetName();
        auto MessageName = GameplayTags::ResolveGameplayTag(MessagePayloadStructName, "Generated from a Message payload definition");
        utils_messaging::BindTo_OnBroadcast(InHandle, MessageName, InBindingPolicy, InPostFireBehavior, InDelegate);
    }
}