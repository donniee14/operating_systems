ALL= leakcount memory_shim.so sctracer
CFLAGS+=-O3 -Wall -Werror -Wextra
CXXFLAGS+=$(CFLAGS) -std=c++11 -fno-exceptions -fno-rtti

.PHONY: all strip clean
all: $(ALL)
strip: $(ALL)
	strip $(ALL)
clean:
	$(RM) $(ALL) $(wildcard *.o)

memory_shim.so: memory_shim.c
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $< -ldl
