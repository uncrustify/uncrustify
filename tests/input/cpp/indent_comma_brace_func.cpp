#include <string>

extern char* externBufferWithAVeryLongName;
extern unsigned int externBufferSizeWithLongName;

std::string foo()
{
    return std::string{ externBufferWithAVeryLongName
        , externBufferSizeWithLongName };
}
