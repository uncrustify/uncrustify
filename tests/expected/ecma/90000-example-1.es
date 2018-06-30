x = 0;            // A global variable
var y = 'Hello!'; // Another global variable

function f()
{
   var z = 'foxes'; // A local variable

   twenty = 20;     // Global because keyword var is not used
   return(x);       // We can use x here because it is global
}
// The value of z is no longer available

