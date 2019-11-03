exports.test = function(libUncrustify, assert){
    var uncrustify = libUncrustify();

    var input = '        string a = "aaaa";';
    var expectedOutput = 'string a = "aaaa";';
    var generatedOutput = uncrustify.uncrustify( input, uncrustify.Language.CPP );

    assert.deepEqual(expectedOutput, generatedOutput, "comparing expectedOutput and generatedOutput");

    uncrustify.destruct();
};

if (module == require.main) {
    if(process.argv.length < 3) {throw "libUncrustify.js path missing";}
	var uncrustify = require(process.argv[2]);
	var assert = require("assert");
	exports.test(uncrustify, assert);
}
