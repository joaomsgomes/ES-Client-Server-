# 🧪 Como Usar os Testes

## Setup Inicial

```bash
cd tests
chmod +x *.sh
```

## Teste Rápido (Milestone 1 e 2)

### 1. Iniciar servidor
```bash
# Terminal 1
cd /home/semed/IST/RC/ES-Client-Server-
make
./ES -p 58001 -v
```

### 2. Executar testes
```bash
# Terminal 2
cd tests
bash quick_test.sh
```

---

## Teste Manual

### Milestone 1: Login/Logout
```bash
# Terminal 1: Servidor
./ES -p 58001 -v

# Terminal 2: Cliente
./user -n 127.0.0.1 -p 58001 < tests/test_m1_login.txt
```

### Milestone 2: Change Password
```bash
./user -n 127.0.0.1 -p 58001 < tests/test_m2_changepass.txt
```

### Milestone 3: Criar Evento
```bash
./user -n 127.0.0.1 -p 58001 < tests/test_m3_create.txt
```

### Milestone 4: Reservar
```bash
# Primeiro cria evento (terminal 2)
./user -n 127.0.0.1 -p 58001 < tests/test_m3_create.txt

# Depois reserva (terminal 3)
./user -n 127.0.0.1 -p 58001 < tests/test_m4_reserve.txt
```

---

## Teste de Concorrência

```bash
# Terminal 1: Servidor
./ES -p 58001 -v

# Terminal 2: Teste
cd tests
bash test_concurrency.sh
```

**O que testa:** 20 clientes tentam reservar simultaneamente 1 lugar cada num evento com 10 lugares. Apenas 10 devem conseguir.

---

## Teste Interativo (Debugging)

```bash
# Terminal 1: Servidor
./ES -p 58001 -v

# Terminal 2: Cliente interativo
./user -n 127.0.0.1 -p 58001

# Depois escreve comandos manualmente:
> login 123456 pass1234
> myevents
> logout
> exit
```

---

## Ficheiros de Teste

- `test_m1_login.txt` - Login e logout básico
- `test_m1_login_wrong.txt` - Testar password errada
- `test_m2_changepass.txt` - Alterar password
- `test_m2_myevents.txt` - Listar eventos vazios
- `test_m3_create.txt` - Criar evento
- `test_m4_reserve.txt` - Fazer reserva
- `event.txt` - Ficheiro de evento de teste

---

## Ordem Recomendada

1. **Semana 1:** `test_m1_login.txt`
2. **Semana 2:** `test_m2_changepass.txt`, `test_m2_myevents.txt`
3. **Semana 3:** `test_m3_create.txt`
4. **Semana 4:** `test_m4_reserve.txt`, `test_concurrency.sh`

---

## Troubleshooting

### Erro: "Connection refused"
- Certifica-te que o servidor está a correr
- Verifica a porta (58001)

### Erro: "Address already in use"
```bash
lsof -i :58001
kill -9 [PID]
```

### Limpar dados do servidor
```bash
rm -rf data/
```
