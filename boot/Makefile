BUILD_DIR?=../build/sim

include $(BUILD_DIR)/software/include/generated/variables.mak
include $(SOC_DIRECTORY)/software/common.mak

SRCS += $(wildcard wolfcrypt/src/*.c)
SRCS += $(wildcard src/*.c)
SRCS += mem_profile.c
OBJECTS  = crt0.o main.o
OBJECTS += $(SRCS:.c=.o)
INC  = -I./wolfssl -I./wolfssl/wolfcrypt -I.

all: boot.bin

$(BUILD_DIR)/software/include/generated/variables.mak $(SOC_DIRECTORY)/software/common.mak:
	cd .. && ./scripts/gen-sim-files.sh

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@
ifneq ($(OS),Windows_NT)
	chmod -x $@
endif

vpath %.a $(PACKAGES:%=../%)

CFLAGS += $(INC) -DWOLFSSL_USER_SETTINGS -Os

boot.elf: $(OBJECTS)
	$(CC) $(LDFLAGS) -T linker.ld -N -o $@ \
		$(OBJECTS) \
		$(PACKAGES:%=-L$(BUILD_DIR)/software/%) \
		-Wl,--whole-archive \
		-Wl,--gc-sections \
		-Wl,-Map,$@.map \
		$(LIBS:lib%=-l%)

ifneq ($(OS),Windows_NT)
	chmod -x $@
endif

# pull in dependency info for *existing* .o files
-include $(OBJECTS:.o=.d)

main.c: certs/CA_der.h certs/client_der.h certs/client_key.h

certs/CA_der.h certs/client_der.h certs/client_key.h:
	cd certs && ./gen-certs.sh

donut.o: CFLAGS   += -w

VPATH = $(BIOS_DIRECTORY):$(BIOS_DIRECTORY)/cmds:$(CPU_DIRECTORY)


%.o: %.cpp
	$(compilexx)

%.o: %.c
	$(compile)

%.o: %.S
	$(assemble)

clean:
	$(RM) $(OBJECTS) $(OBJECTS:.o=.d) boot.elf boot.bin certs/*.der certs/*.h certs/*.key certs/*.pem certs/*.csr certs/*.srl .*~ *~

.PHONY: all clean
