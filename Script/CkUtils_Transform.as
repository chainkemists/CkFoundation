// overloads to make it easier to work with transforms
namespace utils_transform
{
    void
    Request_SetTransform(FCk_Handle InHandle, FTransform InTransform,
        ECk_RelativeAbsolute InRelativeAbsolute = ECk_RelativeAbsolute::Absolute)
    {
        auto _InHandle = InHandle;
        auto Request = FCk_Request_Transform_SetTransform(InTransform);
        Request.Set_RelativeAbsolute(InRelativeAbsolute);
        _InHandle.Request_SetTransform(Request);
    }

    void
    Request_SetScale(FCk_Handle InHandle, FVector InNewScale,
        ECk_LocalWorld InLocalWorld = ECk_LocalWorld::World)
    {
        auto _InHandle = InHandle;
        auto Request = FCk_Request_Transform_SetScale(InNewScale);
        Request.Set_LocalWorld(InLocalWorld);
        _InHandle.Request_SetScale(Request);
    }

    void
    Request_SetRotation(FCk_Handle InHandle, FRotator InNewRotation,
        ECk_LocalWorld InLocalWorld = ECk_LocalWorld::World)
    {
        auto _InHandle = InHandle;
        auto Request = FCk_Request_Transform_SetRotation(InNewRotation);
        Request.Set_LocalWorld(InLocalWorld);
        _InHandle.Request_SetRotation(Request);
    }

    void
    Request_SetLocation(FCk_Handle InHandle, FVector InNewLocation,
        ECk_LocalWorld InLocalWorld = ECk_LocalWorld::World)
    {
        auto _InHandle = InHandle;
        auto Request = FCk_Request_Transform_SetLocation(InNewLocation);
        Request.Set_LocalWorld(InLocalWorld);
        _InHandle.Request_SetLocation(Request);
    }

    void
    Request_SetLocationAndRotation(FCk_Handle InHandle, FVector InNewLocation, FRotator InNewRotation,
        ECk_LocalWorld InLocalWorld = ECk_LocalWorld::World)
    {
        auto _InHandle = InHandle;
        auto Request = FCk_Request_Transform_SetLocationAndRotation(InNewLocation, InNewRotation);
        Request.Set_LocalWorld(InLocalWorld);
        _InHandle.Request_SetLocationAndRotation(Request);
    }

    void
    Request_AddRotationOffset(FCk_Handle InHandle, FRotator InDeltaRotation,
        ECk_LocalWorld InLocalWorld = ECk_LocalWorld::World)
    {
        auto _InHandle = InHandle;
        auto Request = FCk_Request_Transform_AddRotationOffset(InDeltaRotation);
        Request.Set_LocalWorld(InLocalWorld);
        _InHandle.Request_AddRotationOffset(Request);
    }

    void
    Request_AddLocationOffset(FCk_Handle InHandle, FVector InDeltaLocation,
        ECk_LocalWorld InLocalWorld = ECk_LocalWorld::World)
    {
        auto _InHandle = InHandle;
        auto Request = FCk_Request_Transform_AddLocationOffset(InDeltaLocation);
        Request.Set_LocalWorld(InLocalWorld);
        _InHandle.Request_AddLocationOffset(Request);
    }

    FTransform
    Get_EntityCurrentTransform(const FCk_Handle &in InHandle)
    {
        return InHandle.Get_EntityCurrentTransform();
    }

    FVector
    Get_EntityCurrentScale(const FCk_Handle &in InHandle)
    {
        return InHandle.Get_EntityCurrentScale();
    }

    FRotator
    Get_EntityCurrentRotation(const FCk_Handle &in InHandle)
    {
        return InHandle.Get_EntityCurrentRotation();
    }

    FVector
    Get_EntityCurrentLocation(const FCk_Handle &in InHandle)
    {
        return InHandle.Get_EntityCurrentLocation();
    }
}