void *stopper_for_apply = (int[]) {0};
//                               ^ here

template<typename T, typename U>
auto add(T t, U u) -> decltype(t + u) {
//                                   ^ here
	return t + u;
}

void f()noexcept() {
//                ^ here
}

#define FOO5(x) for(;;) (!(x)) { *(volatile int*)0 = 1; }
//                            ^ here

(struct foo) {...}
//          ^ here
