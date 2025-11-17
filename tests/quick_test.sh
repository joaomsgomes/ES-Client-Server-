#!/bin/bash

# Script rápido para testar o projeto

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

PORT=58001

echo "================================"
echo "  Quick Test - Event Reservation"
echo "================================"
echo ""

# Função para testar
run_test() {
    local test_name=$1
    local test_file=$2
    
    echo -e "${YELLOW}Test: ${test_name}${NC}"
    
    timeout 5 ../user -n 127.0.0.1 -p ${PORT} < ${test_file}
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ OK${NC}"
    else
        echo -e "${RED}✗ FAIL${NC}"
    fi
    echo ""
    sleep 1
}

echo -e "${YELLOW}Certifica-te que o servidor está a correr:${NC}"
echo "  ./ES -p 58001 -v"
echo ""
read -p "Pressiona ENTER para continuar..."
echo ""

# Executar testes
run_test "Login/Logout" "test_m1_login.txt"
run_test "Login Wrong Password" "test_m1_login_wrong.txt"
run_test "Change Password" "test_m2_changepass.txt"
run_test "My Events (empty)" "test_m2_myevents.txt"

echo ""
echo "================================"
echo "  Para testar CREATE/RESERVE:"
echo "================================"
echo ""
echo "1. Terminal 1 (Servidor):"
echo "   ./ES -p 58001 -v"
echo ""
echo "2. Terminal 2 (Criar evento):"
echo "   ./user -n 127.0.0.1 -p 58001 < tests/test_m3_create.txt"
echo ""
echo "3. Terminal 3 (Fazer reserva):"
echo "   ./user -n 127.0.0.1 -p 58001 < tests/test_m4_reserve.txt"
echo ""
