// Test: align_func_proto_span must not lose the pending alignments when a
// deeper parenthesis level is met at a lower brace level

namespace outer {
class Widget {
public:
void draw();
unsigned int width() const;
int height() const { return h; }
long area() const { return w * h; }
};
}
int value = ((((1 + 2) * 3) - 4) / 5);

namespace other {
struct Point {
double distance(const Point& p) const;
int x() const;
};
int check = (((1)));
}

int last_one();
long last_two(int a);
