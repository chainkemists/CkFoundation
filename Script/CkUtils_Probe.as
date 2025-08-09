namespace utils_probe
{
    FCk_Handle_Probe
    Add_Sphere(FCk_Handle_Transform InHandle, float32 InRadius, const FCk_Fragment_Probe_ParamsData &in InParams, FCk_Probe_DebugInfo InDebugInfo = FCk_Probe_DebugInfo())
    {
        auto Dimensions = FCk_ShapeSphere_Dimensions(InRadius);
        auto ShapeParams = FCk_Fragment_ShapeSphere_ParamsData(Dimensions);
        utils_shape_sphere::Add(InHandle, ShapeParams);

        auto HandleCopy = InHandle;
        return UCk_Utils_Probe_UE::Add(HandleCopy, InParams, InDebugInfo);
    }

    FCk_Handle_Probe
    Add_Box(FCk_Handle_Transform InHandle, FVector InHalfExtents, const FCk_Fragment_Probe_ParamsData &in InParams, FCk_Probe_DebugInfo InDebugInfo = FCk_Probe_DebugInfo())
    {
        auto Dimensions = FCk_ShapeBox_Dimensions(InHalfExtents);
        auto ShapeParams = FCk_Fragment_ShapeBox_ParamsData(Dimensions);
        utils_shape_box::Add(InHandle, ShapeParams);

        auto HandleCopy = InHandle;
        return UCk_Utils_Probe_UE::Add(HandleCopy, InParams, InDebugInfo);
    }

    FCk_Handle_Probe
    Add_Capsule(FCk_Handle_Transform InHandle, float32 InHalfHeight, float32 InRadius, const FCk_Fragment_Probe_ParamsData &in InParams, FCk_Probe_DebugInfo InDebugInfo = FCk_Probe_DebugInfo())
    {
        auto Dimensions = FCk_ShapeCapsule_Dimensions(InHalfHeight, InRadius);
        auto ShapeParams = FCk_Fragment_ShapeCapsule_ParamsData(Dimensions);
        utils_shape_capsule::Add(InHandle, ShapeParams);

        auto HandleCopy = InHandle;
        return UCk_Utils_Probe_UE::Add(HandleCopy, InParams, InDebugInfo);
    }

    FCk_Handle_Probe
    Add_Cylinder(FCk_Handle_Transform InHandle, float32 InHalfHeight, float32 InRadius, const FCk_Fragment_Probe_ParamsData &in InParams, FCk_Probe_DebugInfo InDebugInfo = FCk_Probe_DebugInfo())
    {
        auto Dimensions = FCk_ShapeCylinder_Dimensions(InHalfHeight, InRadius);
        auto ShapeParams = FCk_Fragment_ShapeCylinder_ParamsData(Dimensions);
        utils_shape_cylinder::Add(InHandle, ShapeParams);

        auto HandleCopy = InHandle;
        return UCk_Utils_Probe_UE::Add(HandleCopy, InParams, InDebugInfo);
    }
}
