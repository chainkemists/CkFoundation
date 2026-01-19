namespace utils_probe
{
    FCk_Handle_Probe
    Add_Sphere(FCk_Handle_Transform InHandle, float32 InRadius, const FCk_Fragment_Probe_ParamsData &in InParams, FCk_Probe_DebugInfo InDebugInfo = FCk_Probe_DebugInfo())
    {
        auto Dimensions = FCk_ShapeSphere_Dimensions(InRadius);
        auto ShapeParams = FCk_Fragment_ShapeSphere_ParamsData(Dimensions);
        utils_shape_sphere::Add(InHandle, ShapeParams);

        return utils_probe::Add(InHandle, InParams, InDebugInfo);
    }

    FCk_Handle_Probe
    Add_Box(FCk_Handle_Transform InHandle, FVector InHalfExtents, const FCk_Fragment_Probe_ParamsData &in InParams, FCk_Probe_DebugInfo InDebugInfo = FCk_Probe_DebugInfo())
    {
        auto Dimensions = FCk_ShapeBox_Dimensions(InHalfExtents);
        auto ShapeParams = FCk_Fragment_ShapeBox_ParamsData(Dimensions);
        utils_shape_box::Add(InHandle, ShapeParams);

        return utils_probe::Add(InHandle, InParams, InDebugInfo);
    }

    FCk_Handle_Probe
    Add_Capsule(FCk_Handle_Transform InHandle, float32 InHalfHeight, float32 InRadius, const FCk_Fragment_Probe_ParamsData &in InParams, FCk_Probe_DebugInfo InDebugInfo = FCk_Probe_DebugInfo())
    {
        auto Dimensions = FCk_ShapeCapsule_Dimensions(InHalfHeight, InRadius);
        auto ShapeParams = FCk_Fragment_ShapeCapsule_ParamsData(Dimensions);
        utils_shape_capsule::Add(InHandle, ShapeParams);

        return utils_probe::Add(InHandle, InParams, InDebugInfo);
    }

    FCk_Handle_Probe
    Add_Cylinder(FCk_Handle_Transform InHandle, float32 InHalfHeight, float32 InRadius, const FCk_Fragment_Probe_ParamsData &in InParams, FCk_Probe_DebugInfo InDebugInfo = FCk_Probe_DebugInfo())
    {
        auto Dimensions = FCk_ShapeCylinder_Dimensions(InHalfHeight, InRadius);
        auto ShapeParams = FCk_Fragment_ShapeCylinder_ParamsData(Dimensions);
        utils_shape_cylinder::Add(InHandle, ShapeParams);

        return utils_probe::Add(InHandle, InParams, InDebugInfo);
    }

    FCk_Handle_Probe
    Request_ResizeCylinder(FCk_Handle_Probe& InHandle, float32 InHalfHeight, float32 InRadius)
    {
        auto ProbeShapeHandle = InHandle.As_ShapeCylinder();
        auto Dimensions = FCk_ShapeCylinder_Dimensions(InHalfHeight, InRadius);
        utils_shape_cylinder::Request_UpdateDimensions(ProbeShapeHandle, FCk_Request_ShapeCylinder_UpdateDimensions(Dimensions));

        return InHandle;
    }

    FCk_Handle_Probe
    Request_ResizeCapsule(FCk_Handle_Probe& InHandle, float32 InHalfHeight, float32 InRadius)
    {
        auto ProbeShapeHandle = InHandle.As_ShapeCapsule();
        auto Dimensions = FCk_ShapeCapsule_Dimensions(InHalfHeight, InRadius);
        utils_shape_capsule::Request_UpdateDimensions(ProbeShapeHandle, FCk_Request_ShapeCapsule_UpdateDimensions(Dimensions));

        return InHandle;
    }

    FCk_Handle_Probe
    Request_ResizeBox(FCk_Handle_Probe& InHandle, FVector InHalfExtents)
    {
        auto ProbeShapeHandle = InHandle.As_ShapeBox();
        auto Dimensions = FCk_ShapeBox_Dimensions(InHalfExtents);
        utils_shape_box::Request_UpdateDimensions(ProbeShapeHandle, FCk_Request_ShapeBox_UpdateDimensions(Dimensions));

        return InHandle;
    }

    FCk_Handle_Probe
    Request_ResizeSphere(FCk_Handle_Probe& InHandle, float32 InRadius)
    {
        auto ProbeShapeHandle = InHandle.As_ShapeSphere();
        auto Dimensions = FCk_ShapeSphere_Dimensions(InRadius);
        utils_shape_sphere::Request_UpdateDimensions(ProbeShapeHandle, FCk_Request_ShapeSphere_UpdateDimensions(Dimensions));

        return InHandle;
    }
}
