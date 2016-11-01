// various combos from unity

contents.Append( "#include \"{file.GetBoundPath ()}\"" );
contents.Append( $"#include \"{file.GetBoundPath ()}\"" );
contents.Append( $"#include \"{file.GetBoundPath ("abc def")}\"" );

contents.Append(
    $"#include \"{file.GetBoundPath()}\"");

contents.Append( $@"#include
""{file.GetBoundPath($@"abc 
 def")}""" );

contents.Append(
    $@"#include
""{file.GetBoundPath(@"abc
   def ghi")}""" );

contents. Append( @"#include
""{file.GetBoundPath ()}""" );

// fun with nesting

$@"{$"\\\"abc{$@" \""def\"" {"{ghi}"} {jkl} {{mno}}"}\\\""
}".Dump();
    
$@"{$@"\""abc{$@" def {"{ghi}"}
    {jkl} {{mno}}\"""}"
}".Dump();

// ensure we didn't break @for etc

var @for = @base + @this - $@"{@while}"  ;

// from roslyn's InterpolationTests.cs

Console.WriteLine($"{number}");

Console.WriteLine($"{number}{number}");
Console.WriteLine($"Jenny don\'t change your number { number :###-####} { number :###-####}.");
Console.WriteLine($"jenny { ((Func<int>)(() => { return number; })).Invoke() :(408) ###-####}");
Console.WriteLine( $"{hello}, { world }." );

Console.WriteLine( $@"{
                            hello
                    },
{
                    world }." );

System.Console.Write($"{{ x }}");
var s = $@"{$@""{1}""}";

Console.WriteLine($"{ await hello }, { await world }!");

Console.WriteLine($"X = { 123 , -(3+4) }.");

var s1 = $"X = { 1 } ";
