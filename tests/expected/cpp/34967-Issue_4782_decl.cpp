// Issue #4782, part A: a '^' immediately after a template's closing '>'
// must not be misidentified as an Objective-C block literal.

array<int>^ GetData()
{
    return m_Data;
}

array<int>^ m_Data;

String^ GetName()
{
    return m_Name;
}

List<String^>^ GetNames()
{
    return m_Names;
}

List<List<int> >^ nested;

static array<int>^ s_Data;

class Foo
{
array<int>^ m_Data;
static array<int>^ s_Data;
};
