#!/bin/bash
# Compila el firmware REAL (CoffeeScale.ino) contra piezas falsas de Arduino y lo ejecuta con un cafe simulado.
# No sustituye a probarlo en la placa: comprueba la logica (modos, tiempo, parada, menu, avisos, atenuacion).
set -e
cd "$(dirname "$0")"
F=../CoffeeScale
cp $F/CoffeeScale.ino CoffeeScale_sketch.inc
cp $F/*.h .
g++ -std=gnu++17 -O1 -Wall -Wno-unused-variable -Wno-unused-function -DUSER_SETUP_ID=147 -I stubs -I . test.cpp -o test_host
./test_host
