const auto test = [ = ] (NSString *param) {};
const auto test =       [&](NSString *param) {};
const auto test = []           (NSString *param) {};
func([=](NSString *     param) {});
func([&](NSString *param) {});
func([](NSString *param) {});
func(param1, [=](NSString *param) {});
