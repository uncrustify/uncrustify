void test(void) {
	auto const ic = 1;
	auto iv = 1;
	auto const& ric = ic;
	auto& riv = iv;
	const auto& ric2 = ic;
	if (auto const& r(ric); r > 0) {
	}
	if (auto& r(riv); r > 0) {
	}
}
