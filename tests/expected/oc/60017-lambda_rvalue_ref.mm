void f() {
	auto provider = std::make_shared<Provider>([weakSelf](std::function<void()> &&onRender, bool isAsync) {});
}
