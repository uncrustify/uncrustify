var dude = "Dude";
var dude2 = new { Name = "Dude", Age = 30, };
var dude3 = new { Name = "Dude", Age = 30, Kids = new { Name = "LittleDude" } };
var dude4 = new { Name = "Dude", Age = 30, Kids = new[] { "LittleDude" } };
var dude5 = new { Name = "Dude", Age = 30, Kids = new[] { new { Name = "LittleDude" } } };
Action y = () => { };
Func<int, float, bool> z = (a, b) => { var z = new { a, b }; return z == null; };
