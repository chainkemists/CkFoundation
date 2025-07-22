// overloads to make it easier to work with transforms
namespace utils_transform
{
    void
    Request_SetTransform(FCk_Handle InHandle, FTransform InTransform,
        ECk_RelativeAbsolute InRelativeAbsolute = ECk_RelativeAbsolute::Absolute)
    {
        auto _InHandle = InHandle;
        auto Request = FCk_Request_Transform_SetTransform();
        Request._NewTransform = InTransform;
        Request._RelativeAbsolute = InRelativeAbsolute;
        UCk_Utils_Transform_TypeUnsafe_UE::Request_SetTransform(_InHandle, Request);
    }

    void
    Request_SetScale(FCk_Handle InHandle, FVector InNewScale,
        ECk_LocalWorld InLocalWorld = ECk_LocalWorld::World)
    {
        auto _InHandle = InHandle;
        auto Request = FCk_Request_Transform_SetScale();
        Request._NewScale = InNewScale;
        Request._LocalWorld = InLocalWorld;
        UCk_Utils_Transform_TypeUnsafe_UE::Request_SetScale(_InHandle, Request);
    }

    void
    Request_SetRotation(FCk_Handle InHandle, FRotator InNewRotation,
        ECk_LocalWorld InLocalWorld = ECk_LocalWorld::World)
    {
        auto _InHandle = InHandle;
        auto Request = FCk_Request_Transform_SetRotation();
        Request._NewRotation = InNewRotation;
        Request._LocalWorld = InLocalWorld;
        UCk_Utils_Transform_TypeUnsafe_UE::Request_SetRotation(_InHandle, Request);
    }

    void
    Request_SetLocation(FCk_Handle InHandle, FVector InNewLocation,
        ECk_LocalWorld InLocalWorld = ECk_LocalWorld::World)
    {
        auto _InHandle = InHandle;
        auto Request = FCk_Request_Transform_SetLocation();
        Request._NewLocation = InNewLocation;
        Request._LocalWorld = InLocalWorld;
        UCk_Utils_Transform_TypeUnsafe_UE::Request_SetLocation(_InHandle, Request);
    }

    void
    Request_SetLocationAndRotation(FCk_Handle InHandle, FVector InNewLocation, FRotator InNewRotation,
        ECk_LocalWorld InLocalWorld = ECk_LocalWorld::World)
    {
        auto _InHandle = InHandle;
        auto Request = FCk_Request_Transform_SetLocationAndRotation();
        Request._NewLocation = InNewLocation;
        Request._NewRotation = InNewRotation;
        Request._LocalWorld = InLocalWorld;
        UCk_Utils_Transform_TypeUnsafe_UE::Request_SetLocationAndRotation(_InHandle, Request);
    }

    void
    Request_AddRotationOffset(FCk_Handle InHandle, FRotator InDeltaRotation,
        ECk_LocalWorld InLocalWorld = ECk_LocalWorld::World)
    {
        auto _InHandle = InHandle;
        auto Request = FCk_Request_Transform_AddRotationOffset();
        Request._DeltaRotation = InDeltaRotation;
        Request._LocalWorld = InLocalWorld;
        UCk_Utils_Transform_TypeUnsafe_UE::Request_AddRotationOffset(_InHandle, Request);
    }

    void
    Request_AddLocationOffset(FCk_Handle InHandle, FVector InDeltaLocation,
        ECk_LocalWorld InLocalWorld = ECk_LocalWorld::World)
    {
        auto _InHandle = InHandle;
        auto Request = FCk_Request_Transform_AddLocationOffset();
        Request._DeltaLocation = InDeltaLocation;
        Request._LocalWorld = InLocalWorld;
        UCk_Utils_Transform_TypeUnsafe_UE::Request_AddLocationOffset(_InHandle, Request);
    }

    FTransform
    Get_EntityCurrentTransform(const FCk_Handle &in InHandle)
    {
        return UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentTransform(InHandle);
    }

    FVector
    Get_EntityCurrentScale(const FCk_Handle &in InHandle)
    {
        return UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentScale(InHandle);
    }

    FRotator
    Get_EntityCurrentRotation(const FCk_Handle &in InHandle)
    {
        return UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentRotation(InHandle);
    }

    FVector
    Get_EntityCurrentLocation(const FCk_Handle &in InHandle)
    {
        return UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentLocation(InHandle);
    }
}