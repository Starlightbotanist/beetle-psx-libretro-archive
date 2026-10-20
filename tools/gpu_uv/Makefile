ROOT := ../..
CFLAGS ?= -O2 -Wall -Wextra -std=gnu89 -fwrapv
CPPFLAGS := -I$(ROOT) -I$(ROOT)/libretro-common/include
SERIALIZATION_CPPFLAGS := -I$(ROOT)/mednafen -I$(ROOT)/include -DMEDNAFEN_VERSION_NUMERIC=0
SERIALIZATION_CFLAGS := $(filter-out -std=%,$(CFLAGS)) -std=gnu99 -flto -ffunction-sections -fdata-sections
# Match the core's warning exceptions for its existing GPU/state sources.
SERIALIZATION_CFLAGS += -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-sign-compare -Wno-strict-aliasing
SERIALIZATION_LDFLAGS := -Wl,--gc-sections
ifeq ($(OS),Windows_NT)
EXEEXT := .exe
SERIALIZATION_LDFLAGS += -Wl,--exclude-all-symbols
endif

SRC := geometry.c $(ROOT)/mednafen/psx/gpu_polygon_sub.c
DECODER_SRC := decoder.c $(ROOT)/mednafen/psx/gpu_polygon_sub.c $(ROOT)/pgxp/pgxp_gpu.c $(ROOT)/mednafen/state.c $(ROOT)/libretro-common/compat/compat_strl.c
SERIALIZATION_SRC := serialization.c $(ROOT)/mednafen/psx/gpu.c $(ROOT)/mednafen/state.c $(ROOT)/libretro-common/compat/compat_strl.c

all: geometry$(EXEEXT) serialization$(EXEEXT) decoder$(EXEEXT)

geometry$(EXEEXT): $(SRC) $(ROOT)/mednafen/psx/gpu.h $(ROOT)/rhi/rhi_intf.h $(ROOT)/beetle_psx_globals.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(SRC)

serialization$(EXEEXT): test_stubs.h $(SERIALIZATION_SRC) $(wildcard $(ROOT)/mednafen/psx/gpu*.h $(ROOT)/mednafen/psx/gpu*.c) $(ROOT)/mednafen/state.h $(ROOT)/mednafen/state_helpers.h
	$(CC) $(CPPFLAGS) $(SERIALIZATION_CPPFLAGS) $(SERIALIZATION_CFLAGS) -o $@ $(SERIALIZATION_SRC) $(SERIALIZATION_LDFLAGS)

decoder$(EXEEXT): $(DECODER_SRC) test_stubs.h $(wildcard $(ROOT)/mednafen/psx/gpu*.h $(ROOT)/mednafen/psx/gpu*.c) $(ROOT)/pgxp/pgxp_gpu.h
	$(CC) $(CPPFLAGS) $(SERIALIZATION_CPPFLAGS) $(SERIALIZATION_CFLAGS) -o $@ $(DECODER_SRC) $(SERIALIZATION_LDFLAGS)

check: all
	./geometry$(EXEEXT)
	./serialization$(EXEEXT)
	./decoder$(EXEEXT)

clean:
	rm -f geometry geometry.exe serialization serialization.exe decoder decoder.exe

.PHONY: all check clean
