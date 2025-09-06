UCLASS()
class UCk_Boolean_InputAction : UInputAction
{
    default ValueType = EInputActionValueType::Boolean;
    default bConsumeInput = false;
};

UCLASS()
class UCk_Axis2d_InputAction : UInputAction
{
    default ValueType = EInputActionValueType::Axis2D;
    default bConsumeInput = false;
};

UCLASS()
class UCk_Default_InputMappingContext : UInputMappingContext
{
};