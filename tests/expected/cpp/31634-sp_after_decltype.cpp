int x;
char y;
auto x1 = decltype(x){0};
auto y1 = decltype(y){'a'};

unsigned rows;
for (auto row = decltype(rows){0}; row < rows; ++row) {
}
