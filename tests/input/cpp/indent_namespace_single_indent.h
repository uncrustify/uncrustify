namespace ns1 {
	namespace ns2 {
		namespace ns3 {
			void a();
		}
	}
}

extern "C" {
	namespace ns1 {
		namespace ns2 {
			namespace ns3 {
				void b();
			}
		}
	}
}

namespace ns1 {
	extern "C" {
		namespace ns2 {
			namespace ns3 {
				void c();
			}
		}
	}
}

namespace ns1 {
	namespace ns2 {
		extern "C" {
			namespace ns3 {
				void d();
			}
		}
	}
}

namespace ns1 {
	namespace ns2 {
		namespace ns3 {
			extern "C" {
				void e();
			}
		}
	}
}

#define M1(ns1, ns2, ns3, f) \
	namespace ns1 { \
		namespace ns2 { \
			namespace ns3 { \
				void f(); \
			} \
		} \
	}

#define M2(ns1, ns2, ns3, f) \
	extern "C" { \
		namespace ns1 { \
			namespace ns2 { \
				namespace ns3 { \
					void b(); \
				} \
			} \
		} \
	}

#define M3(ns1, ns2, ns3, f) \
	namespace ns1 { \
		extern "C" { \
			namespace ns2 { \
				namespace ns3 { \
					void c(); \
				} \
			} \
		} \
	}

#define M4(ns1, ns2, ns3, f) \
	namespace ns1 { \
		namespace ns2 { \
			extern "C" { \
				namespace ns3 { \
					void d(); \
				} \
			} \
		} \
	}

#define M5(ns1, ns2, ns3, f) \
	namespace ns1 { \
		namespace ns2 { \
			namespace ns3 { \
				extern "C" { \
					void e(); \
				} \
			} \
		} \
	}
