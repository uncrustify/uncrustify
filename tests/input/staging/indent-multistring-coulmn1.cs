//This
var a = hello(
    @"world"
);

//should stay the same.
//But this
var a = hello(
    @"world
");

//should get formatted to this
var a = hello(
@"world
");
 