foo(
1
);

foo(bar(
1
)
);

foo(bar(baz(
1)
)
);

foo(bar(
baz(
1)
)
);

foo(
bar(baz(
1)
)
);

foo(
bar(
baz(
1)
)
);

foo(
1,
bar(
2,
baz(
3
)
)
);

foo(1,
bar(2,
3
)
);

namespace ns
{
foo(1
, 2
, [](a, b) {
bar(3
, 4
);
});
}
