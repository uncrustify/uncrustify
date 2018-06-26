// Regression 1 FAKE_METHOD expands to a function prototype. Could possibly use PROTO_WRAP like for FAKE_FUNCTION
class Foo
{
    FAKE_FUNCTION(Bar, GetBarInfo, const BarInfo &());
    FAKE_METHOD(Bar, GetBarInfo, const BarInfo &());
    FAKE_FUNCTION_WITH_LOCAL_NAME(FakeGetCommonScriptingClasses, GetCommonScriptingClasses, const CommonScriptingClasses &());
}
