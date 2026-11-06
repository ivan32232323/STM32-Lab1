##########################################################################
# Кореневий Makefile — обгортка над Docker (ЛР1, форма B / Wokwi)
#   make image    — зібрати тулчейн-образ (один раз)
#   make build    — скомпілювати прошивку в контейнері
#   make flash    — "прошивка" для Wokwi = свіжий ELF + підказка перезапуску
#   make monitor  — UART (для форми A: picocom; для Wokwi — Serial Monitor)
#   make clean
##########################################################################
IMAGE ?= stm32-build
ELF    = firmware/build/firmware.elf
PORT  ?= /dev/ttyACM0

ifeq ($(OS),Windows_NT)
  DOCKER_USER :=
  PWD_HOST    := $(CURDIR)
else
  DOCKER_USER := --user $(shell id -u):$(shell id -g)
  PWD_HOST    := $(CURDIR)
endif

DOCKER_RUN = docker run --rm -v "$(PWD_HOST)":/workspace -w /workspace $(DOCKER_USER) $(IMAGE)

image:
	docker build -t $(IMAGE) docker/

build:
	$(DOCKER_RUN) make -C firmware all

clean:
	$(DOCKER_RUN) make -C firmware clean

flash: build
	@echo "ELF готовий: $(ELF)"
	@echo "Wokwi: F1 -> 'Wokwi: Restart Simulator' (wokwi.toml вже вказує на ELF)."
	@echo "Форма A: st-flash --reset write firmware/build/firmware.bin 0x08000000"

monitor:
	@echo "Wokwi: вивід USART2 у панелі Serial Monitor симулятора."
	@echo "Форма A: picocom -b 115200 --imap lfcrlf $(PORT)"

.PHONY: image build clean flash monitor
