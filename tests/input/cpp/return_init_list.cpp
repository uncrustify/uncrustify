inline static std::tuple<bool, std::string> foo(void) {
// should remain a one liner
return{true, ""s};
}
inline static std::tuple<bool, std::string, std::string> foo(void) {
if (condition) {
// should remain a one liner
return{true, ""s, ""s};
}
// should remain a one liner
return{false, ""s, ""s};
}
inline static std::tuple<bool, std::string> foo(void) {
// should indent one level
return{
true, ""s
};
}
inline static std::tuple<bool, std::string> foo(void) {
// should indent one level on new line
return
{ true, ""s };
}
inline static std::tuple<bool, std::string> foo(void) {
// should indent one level for braces and another level for values
return
{
true, ""s
};
}
