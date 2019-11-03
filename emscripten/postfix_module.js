    //! auto initializes the module
    Module["_initialize"](); //// execute this only one time,
    Module["_initialize"] = Function.prototype; // and replace it with a noop

    /**
     * Takes in a JS string with other params, converts it to UTF8 and
     * passes it to the actual _uncrustify function while also managing the
     * memory on the emscripten heap.
     */
    Module["uncrustify"] = function(str, langIDX, frag, defer)
    {
        if( !str || typeof(str) !== "string" || str.length === 0 ) {return "";}

        var nDataBytes = lengthBytesUTF8(str)+1; // +1 for \0
        var stringInputPtr = Module._malloc(nDataBytes);
        Module.stringToUTF8(str, stringInputPtr, nDataBytes);

        var retStringPointer = 0;

        switch(arguments.length)
        {
            // depending in the number of args the internal select_overload
            // function resolves the appropriate internal _uncrustify function
            case 2:
            {
                retStringPointer = Module["_uncrustify"](stringInputPtr, langIDX);
                break;
            }
            case 3:
            {
                retStringPointer = Module["_uncrustify"](stringInputPtr, langIDX, frag);
                break;
            }
//             case 4:
//             {
//                 retStringPointer = Module["_uncrustify"](stringInputPtr, langIDX, frag, defer);
//                 break;
//             }
            default:
            {
                break;
            }
        }

        Module._free(stringInputPtr);


        var retString = "";

        if(retStringPointer !== 0)
        {
            retString = Module.UTF8ToString(retStringPointer);
            Module._free(retStringPointer);
        }

        return retString;
    }

    /**
     * Takes in a JS string with other params, converts it to UTF8 and
     * passes it to the actual _debug function while also managing the
     * memory on the emscripten heap.
     */
    Module["debug"] = function(str, langIDX, frag)
    {
        if( !str || typeof(str) !== "string" || str.length === 0 ) {return "";}

        var nDataBytes = lengthBytesUTF8(str)+1; // +1 for \0
        var stringInputPtr = Module._malloc(nDataBytes);
        Module.stringToUTF8(str, stringInputPtr, nDataBytes);

        var retStringPointer = 0;

        switch(arguments.length)
        {
            // depending in the number of args the internal select_overload
            // function resolves the appropriate internal _uncrustify function
            case 2:
            {
                retStringPointer = Module["_debug"](stringInputPtr, langIDX);
                break;
            }
            case 3:
            {
                retStringPointer = Module["_debug"](stringInputPtr, langIDX, frag);
                break;
            }
            default:
            {
                break;
            }
        }

        Module._free(stringInputPtr);


        var retString = "";

        if(retStringPointer !== 0)
        {
            retString = Module.UTF8ToString(retStringPointer);
            Module._free(retStringPointer);
        }

        return retString;
    }

    /**
     * Takes in a JS string, removes non ascii chars (only those are needed
     * in a config) and passes it to the actual _loadConfig function while
     * also managing the memory on the emscripten heap.
     */
    Module.load_config = function(str)
    {
        // UTF8 functions return on empty string but they have to be accepted too
        // to reset the current config
        if( !str || typeof(str) !== "string" || str.length === 0) {str = " ";}

        //remove unneeded non asci chars in the config
        str.replace(/[^\x00-\x7F]/g, "");

        var nDataBytes = str.length+1; // +1 for \0
        var stringInputPtr = Module._malloc(nDataBytes);
        Module.writeAsciiToMemory(str, stringInputPtr);


        var retStringPointer = Module["_load_config"](stringInputPtr);
        Module._free(stringInputPtr);


        var retString = "";

        if(retStringPointer !== 0)
        {
            retString = Module.Pointer_stringify(retStringPointer);
            Module._free(retStringPointer);
        }

        return retString;
    }

