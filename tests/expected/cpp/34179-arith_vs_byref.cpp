A a = {this->r & cos(b)};

B b1 = {0x0000'1111 & this->r};
B b2 = {this->r & 0x0000'1111};
B b3 = {0x0000'1111 & value};
B b4 = {value & 0x0000'1111};

auto p = std::make_pair(r & cos(a), r & sin(a));

auto p2 = std::make_pair(r & 0x0000'1111, 0x0000'1111 & r);
