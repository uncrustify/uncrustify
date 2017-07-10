// note that this only happens with positive int literals. if i use a float with a decimal, or 'c' or "abc" or whatever, or even `(-5)`, it's ok.
5.Clamp(2, 10).ShouldBe(5);
5.Clamp(-5, 10).ShouldBe(5);
5.Clamp("a", 10).ShouldBe(5);

"4".Clamp(2, 10).ShouldBe(5);
(-5).Clamp(2, 10).ShouldBe(5);