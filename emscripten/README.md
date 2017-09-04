building
--------------------------------------------------------------------------------
 **dependencies**: cmake, python, clang, emscripten, node
 
 1. create a `build` directory somewhere
 2. inside this directory call `cmake <CMakeLists.txt directory>` and `make`<br>
    (the CMakeLists.txt file is located in <uncrustify_root_dir>/emscripten)

Optionally the generated libUncrustify.js can be tested via `make emscripten_test`

_libUncrustify.js_ example usage
--------------------------------------------------------------------------------
1. load module instance:
    ```js
    var uncrustify = libUncrustify();
    ```

2. set option settings either one at a time with:
    ```js
    uncrustify.set_option( "optionNameString", "newOptionValueString" );
    ```

    or a whole bunch via:

    ```js
    uncrustify.loadConfig( "configFileFormatString" )
    ```

3. set the language of the to be formated file string
    ```js
    uncrustify.set_language( languageInt );
    ```

4. format a file string:
    ```js
    var uncrustyFileString = uncrustify.uncrustify( "crustyFileString" );
    ```

5. delete initialized module instance:
    ```js
    uncrustify.destruct();
    ```
