exports.test = function(libUncrustify, assert){
    var uncrustify = libUncrustify();

    var input = '        string a = "aaaa";';
    var expectedOutput = 'string a = "aaaa";';
    var generatedOutput = uncrustify.uncrustify( input );

    assert.deepEqual(expectedOutput, generatedOutput, "comparing expectedOutput and generatedOutput");

    uncrustify.destruct();
};

if (module == require.main) {
	var uncrustify = require('../libUncrustify.js');
	var assert = require("assert");
	exports.test(uncrustify, assert);
}
