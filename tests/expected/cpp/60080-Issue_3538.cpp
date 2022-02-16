class Alpha {};

class Beta
{
public:
Beta(class Alpha alpha) : _alpha(alpha) {
}

void init();

private:
class Alpha _alpha;
};
