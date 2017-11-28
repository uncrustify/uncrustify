void foo()
{
	BSTR test = L"SID";
	CHAR s[]  = "This is a \"test\"";
	CHAR ch   = 'a';
}

/* The 'u8', 'u', and 'U' prefixes */
const char     *s1 = u8"I'm a UTF-8 string.";
const char16_t *s2 = u"This is a UTF-16 string.";
const char32_t *s3 = U"This is a UTF-32 string.";

const char c1 = u8'1';
const char16_t c2 = u'2';
const char32_t c3 = U'4';
const wchar_t c4 = L'w';
const char16_t u = u'\u007f';

OutputType some_variable = "1234"_Suffix;      //Calls the const char * version
OutputType some_variable = u8"1234"_Suffix;    //Calls the const char * version
OutputType some_variable = L"1234"_Suffix;     //Calls the const wchar_t * version
OutputType some_variable = u"1234"_Suffix;     //Calls the const char16_t * version
OutputType some_variable = U"1234"_Suffix;     //Calls the const char32_t * version