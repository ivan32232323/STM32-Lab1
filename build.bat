@echo off
rem Аналог "make build" для Windows без make
docker image inspect stm32-build >nul 2>nul || docker build -t stm32-build docker/
docker run --rm -v "%cd%":/workspace -w /workspace stm32-build make -C firmware all
