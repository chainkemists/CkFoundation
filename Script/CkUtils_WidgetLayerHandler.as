namespace utils_widget_layer_handler
{
    FCk_Handle_WidgetLayerHandler
    Request_PushToLayer(FGameplayTag InLayer, TSoftClassPtr<UCk_UserWidget_UE> InWidgetClass)
    {
        // auto WidgetLayerHandle = utils_widget_layer_handler::Get_WidgetLayerHandler(Gameplay::GetPlayerController(0));
        // return WidgetLayerHandle.Request_PushToLayer(FCk_Request_WidgetLayerHandler_PushToLayer(InLayer, InWidgetClass));
        return FCk_Handle_WidgetLayerHandler();
    }

    FCk_Handle_WidgetLayerHandler
    Request_PushToLayer_Instanced(FGameplayTag InLayer, UCk_UserWidget_UE InWidget)
    {
        // auto WidgetLayerHandle = utils_widget_layer_handler::Get_WidgetLayerHandler(Gameplay::GetPlayerController(0));
        // return WidgetLayerHandle.Request_PushToLayer_Instanced(FCk_Request_WidgetLayerHandler_PushToLayer_Instanced(InLayer, InWidget));
        return FCk_Handle_WidgetLayerHandler();
    }

    FCk_Handle_WidgetLayerHandler
    Request_PopFromLayer(FGameplayTag InLayer)
    {
        // auto WidgetLayerHandle = utils_widget_layer_handler::Get_WidgetLayerHandler(Gameplay::GetPlayerController(0));
        // return WidgetLayerHandle.Request_PopFromLayer(FCk_Request_WidgetLayerHandler_PopFromLayer(InLayer));
        return FCk_Handle_WidgetLayerHandler();
    }

    FCk_Handle_WidgetLayerHandler
    Request_ClearLayer(FGameplayTag InLayer)
    {
        // auto WidgetLayerHandle = utils_widget_layer_handler::Get_WidgetLayerHandler(Gameplay::GetPlayerController(0));
        // return WidgetLayerHandle.Request_ClearLayer(FCk_Request_WidgetLayerHandler_ClearLayer(InLayer));
        return FCk_Handle_WidgetLayerHandler();
    }
}