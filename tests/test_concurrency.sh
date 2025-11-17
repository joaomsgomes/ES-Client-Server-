#!/bin/bash

# Teste de concorrência - 20 clientes tentam reservar 10 lugares

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

PORT=58001
NUM_CLIENTS=20

echo "================================"
echo "  Concurrency Test"
echo "================================"
echo ""
echo "Criar evento com 10 lugares..."
echo ""

# Criar evento
(
    echo "login 100000 ownerxxx"
    echo "create RaceTest tests/event.txt 31-12-2025 10"
    echo "logout"
    echo "exit"
) | ../user -n 127.0.0.1 -p ${PORT} > /dev/null 2>&1

sleep 1

echo "Lançar ${NUM_CLIENTS} clientes simultâneos..."
echo ""

# Lançar clientes em paralelo
for i in $(seq 1 $NUM_CLIENTS); do
    uid=$(printf "%06d" $i)
    (
        echo "login ${uid} pass${uid}"
        echo "reserve 001 1"
        echo "logout"
        echo "exit"
    ) | ../user -n 127.0.0.1 -p ${PORT} > /tmp/client_${i}.out 2>&1 &
done

# Esperar todos terminarem
wait

# Contar resultados
accepted=0
rejected=0

for i in $(seq 1 $NUM_CLIENTS); do
    if grep -q "accepted" /tmp/client_${i}.out; then
        ((accepted++))
    else
        ((rejected++))
    fi
done

echo ""
echo "Resultados:"
echo "  Aceites: ${accepted}"
echo "  Rejeitadas: ${rejected}"
echo ""

if [ $accepted -eq 10 ]; then
    echo -e "${GREEN}✓ PASS: Exatamente 10 reservas aceites (sem overbooking)${NC}"
    rm /tmp/client_*.out
    exit 0
else
    echo -e "${RED}✗ FAIL: Esperado 10, obtido ${accepted}${NC}"
    rm /tmp/client_*.out
    exit 1
fi
