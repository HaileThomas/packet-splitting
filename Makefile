APPS = rx_split rx_standard hello_world

PKGCONF = PKG_CONFIG_PATH=$(HOME)/splitting/dpdk/build/meson-uninstalled pkg-config

CFLAGS += -O3 $(shell $(PKGCONF) --cflags libdpdk)
CFLAGS += -Wno-deprecated-declarations

LDFLAGS += $(shell $(PKGCONF) --libs --static libdpdk)

DPDK_BUILD_DIR = $(HOME)/splting/dpdk/build
LDFLAGS += -Wl,-rpath=$(DPDK_BUILD_DIR)/lib -Wl,-rpath=$(DPDK_BUILD_DIR)/drivers

all: $(APPS)

%: %.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f $(APPS)