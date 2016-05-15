    if( !Module.hasOwnProperty('noInitialRun') ) { Module.noInitialRun = true; }
    if( !Module.hasOwnProperty('noExitRuntime') ) { Module.noExitRuntime = true; }
    if( !Module.hasOwnProperty('print') || typeof Module["print"] != 'function')
    {
        Module.print = (function()
        {
            return function(text) 
            {
                if (arguments.length > 1) text = Array.prototype.slice.call(arguments).join(' ');
                console.log(text);
            };
        })();
    }
    if( !Module.hasOwnProperty('printErr') || typeof Module["printErr"] != 'function')
    {
        Module.printErr = function(text) 
        {
            if (arguments.length > 1) text = Array.prototype.slice.call(arguments).join(' ');
            if (0) 
            { // XXX disabled for safety typeof dump == 'function') {
                dump(text + '\n'); // fast, straight to the real console
            } 
            else 
            {
                console.error(text);
            }
        };
    }
