namespace utils_scene_node
{
    UFUNCTION()
    FVector
    Get_Offset_Location(
        FCk_Handle_SceneNode InHandle)
    {
        return utils_scene_node::Get_Offset(InHandle).Location;
    }

    UFUNCTION()
    FRotator
    Get_Offset_Rotation(
        FCk_Handle_SceneNode InHandle)
    {
        return utils_scene_node::Get_Offset(InHandle).Rotation.Rotator();
    }

    UFUNCTION()
    FVector
    Get_Offset_Scale(
        FCk_Handle_SceneNode InHandle)
    {
        return utils_scene_node::Get_Offset(InHandle).Scale3D;
    }

    UFUNCTION()
    FCk_Handle_SceneNode
    Request_UpdateOffset_Location(
        FCk_Handle_SceneNode& InHandle,
        FVector InNewOffset,
        ECk_RelativeAbsolute InRelativeAbsolute = ECk_RelativeAbsolute::Absolute)
    {
        auto NewOffset = utils_scene_node::Get_Offset(InHandle);

        switch(InRelativeAbsolute)
        {
            case ECk_RelativeAbsolute::Relative:
            {
                NewOffset.SetLocation(NewOffset.GetLocation() + InNewOffset);
                utils_scene_node::Request_UpdateOffset(InHandle, FCk_Request_SceneNode_UpdateRelativeTransform(NewOffset));
                break;
            }
            case ECk_RelativeAbsolute::Absolute:
            {
                NewOffset.SetLocation(InNewOffset);
                utils_scene_node::Request_UpdateOffset(InHandle, FCk_Request_SceneNode_UpdateRelativeTransform(NewOffset));
                break;
            }
        }

        return InHandle;
    }

    UFUNCTION()
    FCk_Handle_SceneNode
    Request_UpdateOffset_Rotation(
        FCk_Handle_SceneNode& InHandle,
        FRotator InNewRotation,
        ECk_RelativeAbsolute InRelativeAbsolute = ECk_RelativeAbsolute::Absolute)
    {
        auto NewOffset = utils_scene_node::Get_Offset(InHandle);

        switch(InRelativeAbsolute)
        {
            case ECk_RelativeAbsolute::Relative:
            {
                NewOffset.SetRotation(NewOffset.GetRotation().Rotator() + InNewRotation);
                utils_scene_node::Request_UpdateOffset(InHandle, FCk_Request_SceneNode_UpdateRelativeTransform(NewOffset));
                break;
            }
            case ECk_RelativeAbsolute::Absolute:
            {
                NewOffset.SetRotation(InNewRotation);
                utils_scene_node::Request_UpdateOffset(InHandle, FCk_Request_SceneNode_UpdateRelativeTransform(NewOffset));
                break;
            }
        }

        return InHandle;
    }

    UFUNCTION()
    FCk_Handle_SceneNode
    Request_UpdateOffset_Scale(
        FCk_Handle_SceneNode& InHandle,
        FVector InNewScale,
        ECk_RelativeAbsolute InRelativeAbsolute = ECk_RelativeAbsolute::Absolute)
    {
        auto NewOffset = utils_scene_node::Get_Offset(InHandle);

        switch(InRelativeAbsolute)
        {
            case ECk_RelativeAbsolute::Relative:
            {
                NewOffset.SetScale3D(NewOffset.GetScale3D() + InNewScale);
                utils_scene_node::Request_UpdateOffset(InHandle, FCk_Request_SceneNode_UpdateRelativeTransform(NewOffset));
                break;
            }
            case ECk_RelativeAbsolute::Absolute:
            {
                NewOffset.SetScale3D(InNewScale);
                utils_scene_node::Request_UpdateOffset(InHandle, FCk_Request_SceneNode_UpdateRelativeTransform(NewOffset));
                break;
            }
        }

        return InHandle;
    }
}