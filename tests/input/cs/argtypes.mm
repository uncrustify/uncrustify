static void WriteArrayToDrag (NSArray* array, NSPasteboard* pboard, NSString* pboardType);
static OSStatus FindProcess (const FSRef* appRef, ProcessSerialNumber *pPSN, NSString* application, bool permissiveSearching);
NSString* MakeNSString(const std::string& string);
