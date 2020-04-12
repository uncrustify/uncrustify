namespace ns1
{
namespace ns2
{
void func0()
{
functionThatTakesALambda( [&] () -> void
{
lambdaBody;
});
functionThatTakesALambda( [&] __device__ () -> void
{
lambdaBody;
});
functionThatTakesALambda( [&] __host__ __device__ () -> void
{
lambdaBody;
});
functionThatTakesALambda( [&] DEVICE_LAMBDA_CONTEXT () -> void
{
lambdaBody;
});
functionThatTakesALambda( [&] HOST_DEVICE_LAMBDA_CONTEXT () -> void
{
lambdaBody;
});
}
}
}
