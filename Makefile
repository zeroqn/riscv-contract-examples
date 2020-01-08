TARGET := riscv64-unknown-elf
CC := $(TARGET)-gcc
LD := $(TARGET)-gcc
CFLAGS := -Os -DCKB_NO_MMU -D__riscv_soft_float -D__riscv_float_abi_soft
LDFLAGS := -lm -Wl,-static -fdata-sections -ffunction-sections -Wl,--gc-sections -Wl,-s
CURRENT_DIR := $(shell pwd)
DOCKER_BUILD := docker run -v $(CURRENT_DIR):/src nervos/ckb-riscv-gnu-toolchain:bionic bash -c

.DEFAULT_GOAL := all

build:
	$(CC) -I./c -o ./build/ballot ./c/ballot.c

docker_build:
	$(DOCKER_BUILD) "cd /src && make build"

test:
	cargo test -- --nocapture

all: docker_build \
	test

.PHONY: \
	build \
	docker_build
