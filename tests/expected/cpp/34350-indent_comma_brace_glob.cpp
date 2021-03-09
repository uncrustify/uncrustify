#include <string>

extern char* externBufferWithAVeryLongName;
extern unsigned int externBufferSizeWithLongName;

std::string foo{ externBufferWithAVeryLongName
               , externBufferSizeWithLongName };
