namespace utils_widget_layer_handler
{
    FCk_Handle_WidgetLayerHandler
    Request_PushToLayer(FGameplayTag InLayer, TSoftClassPtr<UCk_UserWidget_UE> InWidgetClass)
    {
        auto WidgetLayerHandle = utils_widget_layer_handler::Get_WidgetLayerHandler(Gameplay::GetPlayerController(0));
        return WidgetLayerHandle.Request_PushToLayer(FCk_Request_WidgetLayerHandler_PushToLayer(InLayer, InWidgetClass));
    }

    FCk_Handle_WidgetLayerHandler
    Request_PushToLayer_Instanced(FGameplayTag InLayer, UCk_UserWidget_UE InWidget)
    {
        auto WidgetLayerHandle = utils_widget_layer_handler::Get_WidgetLayerHandler(Gameplay::GetPlayerController(0));
        auto Request = FCk_Request_WidgetLayerHandler_PushToLayer_Instanced();
        Request._Layer = InLayer;
        Request._WidgetInstance = InWidget;

        return WidgetLayerHandle.Request_PushToLayer_Instanced(Request);
    }

    FCk_Handle_WidgetLayerHandler
    Request_PopFromLayer(FGameplayTag InLayer)
    {
        auto WidgetLayerHandle = utils_widget_layer_handler::Get_WidgetLayerHandler(Gameplay::GetPlayerController(0));
        return WidgetLayerHandle.Request_PopFromLayer(FCk_Request_WidgetLayerHandler_PopFromLayer(InLayer));
    }

    FCk_Handle_WidgetLayerHandler
    Request_ClearLayer(FGameplayTag InLayer)
    {
        auto WidgetLayerHandle = utils_widget_layer_handler::Get_WidgetLayerHandler(Gameplay::GetPlayerController(0));
        return WidgetLayerHandle.Request_ClearLayer(FCk_Request_WidgetLayerHandler_ClearLayer(InLayer));
    }
}