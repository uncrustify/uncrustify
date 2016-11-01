building
--------------------------------------------------------------------------------
1. compile Uncrustify normally
2. copy the generated headers into the src directory 
   (uncrustify_version.h, config.h, token_names.h)
3. run the build.sh in src/emscripten/ to generate libUncrustify.js

libEmscripten example usage
--------------------------------------------------------------------------------
1. load module instance:
    var uncrustify = libUncrustify();

2. set option settings either one at a time with:
    uncrustify.set_option( "optionNameString", "newOptionValueString" );
   or a whole bunch via:
    uncrustify.loadConfig( "configFileFormatString" )

3. set the language of the to be formated file string
    uncrustify.set_language( languageInt );

4. format a file string:
    var uncrustyFileString = uncrustify.uncrustify( "crustyFileString" );

5. delete initialized module instance:
    uncrustify.destruct();
