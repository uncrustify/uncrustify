#include "sdkconfig.h"

#include <fs/nvs_storage.hpp>
#include <network/interface.hpp>

extern "C" void app_main (void) {
	fs::nvs_storage::initialize ();
	network::interface::initialize ();
}

#include "sdkconfig.h"
#include "esp_wifi.h"
#include "network/interface.hpp"


using namespace network;
void interface::initialize () {
	tcpip_adapter_init ();
}

// ----------------------------------------

namespace A {
class interface {
public:
interface() {
}

~interface() {
}

void foo() {
}
};
}

namespace B {
class interface {
public:
interface();
~interface();
void foo();
};

inline interface::interface() {
}
inline interface::~interface() {
}
inline void interface::foo() {
}
}

namespace C {
class interface {
public:
interface();
~interface();
void foo();
};

interface::interface() {
}
interface::~interface() {
}
void interface::foo() {
}
}

interface ::external_iterface;
