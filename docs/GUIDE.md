# 🎯 Guia Completo de Desenvolvimento - Event Reservation System

**Projeto:** Sistema de Reserva de Eventos (Cliente-Servidor)  
**Data de Início:** 17 Novembro 2025  
**Data de Entrega:** 19 Dezembro 2025, 23:59  
**Grupo:** [Seu Número]  
**Porto Base:** 58000 + GN

---

## 📋 Índice

1. [Visão Geral](#visão-geral)
2. [Arquitetura do Sistema](#arquitetura-do-sistema)
3. [Cronograma Detalhado](#cronograma-detalhado)
4. [Estrutura do Projeto](#estrutura-do-projeto)
5. [Protocolo de Comunicação](#protocolo-de-comunicação)
6. [Checklist de Implementação](#checklist-de-implementação)
7. [Testes e Validação](#testes-e-validação)
8. [Submissão](#submissão)

---

## 🎯 Visão Geral

### O que vamos construir?

Um sistema distribuído de reserva de eventos composto por:

- **Servidor ES (Event Server)**: Processa pedidos, gere eventos e reservas
- **Cliente User**: Interface para utilizadores criarem eventos e fazerem reservas
- **Protocolo UDP**: Para comandos rápidos (login, logout, listagens)
- **Protocolo TCP**: Para transferência de ficheiros e operações complexas

### Funcionalidades Principais

#### 👤 Gestão de Utilizadores
- ✅ Login/Logout (registar novo utilizador automaticamente)
- ✅ Alterar password
- ✅ Unregister (remover conta)

#### 🎫 Gestão de Eventos (Owner)
- ✅ Criar evento (com ficheiro descritivo até 10MB)
- ✅ Fechar evento (parar de aceitar reservas)
- ✅ Listar meus eventos

#### 🎟️ Reservas (User)
- ✅ Listar todos os eventos
- ✅ Ver detalhes de evento (download ficheiro)
- ✅ Fazer reserva (1-999 lugares)
- ✅ Listar minhas reservas

---

## 🏗️ Arquitetura do Sistema (em ARCHITECTURE.md)

### Fluxo de Dados - Exemplo Completo

```
USER A                          ES SERVER                    USER B
  │                                 │                           │
  │  1. LOGIN (UDP)                │                           │
  ├────────────────────────────────>│                           │
  │<── RLI OK                       │                           │
  │                                 │                           │
  │  2. CREATE EVENT (TCP)         │                           │
  ├────────────────────────────────>│                           │
  │    + File transfer              │                           │
  │<── RCE OK 001                   │                           │
  │                                 │                           │
  │                                 │  3. LOGIN (UDP)          │
  │                                 │<─────────────────────────┤
  │                                 │ RLI OK ──────────────────>│
  │                                 │                           │
  │                                 │  4. LIST (TCP)           │
  │                                 │<─────────────────────────┤
  │                                 │ RLS OK 001 Event1... ────>│
  │                                 │                           │
  │                                 │  5. RESERVE (TCP)        │
  │                                 │<─────────────────────────┤
  │                                 │ RRI ACC ─────────────────>│
  │                                 │                           │
  │  6. MYEVENTS (UDP)             │                           │
  ├────────────────────────────────>│                           │
  │<── RME OK 001 1 (5 reserved)    │                           │
  │                                 │                           │
  │  7. CLOSE EVENT (TCP)          │                           │
  ├────────────────────────────────>│                           │
  │<── RCL OK                       │                           │
  │                                 │                           │
```

---

## 📅 Cronograma Detalhado

### **SEMANA 1: 17-23 Novembro** ⚡
**Objetivo:** Setup e Fundações UDP

| Dia | Tarefas | Tempo Estimado | Prioridade |
|-----|---------|----------------|------------|
| **Seg 17/11** | • Criar estrutura de diretórios<br>• Setup Git<br>• Criar Makefile básico | 2h | 🔴 Alta |
| **Ter 18/11** | • Implementar `utils.c` (validações)<br>• Testar validações | 3h | 🔴 Alta |
| **Qua 19/11** | • Implementar `protocol.c` (UDP)<br>• Funções send/receive UDP | 3h | 🔴 Alta |
| **Qui 20/11** | • Implementar `es_database.c` (estruturas)<br>• User management básico | 4h | 🔴 Alta |
| **Sex 21/11** | • Implementar `es_udp.c`<br>• handle_login() e handle_logout() | 4h | 🔴 Alta |
| **Sáb 22/11** | • Implementar `user_client.c` (main)<br>• cmd_login() e cmd_logout() | 4h | 🔴 Alta |
| **Dom 23/11** | • **TESTE MILESTONE 1**<br>• Login/Logout funcional<br>• Bug fixing | 3h | 🔴 Alta |

**✅ Milestone 1:** Cliente conecta, faz login e logout via UDP

**Comandos funcionais:**
```bash
> login 123456 pass1234
New user registered and logged in.
> logout
Logout successful.
```

---

### **SEMANA 2: 24-30 Novembro** 🚀
**Objetivo:** Completar UDP + Iniciar TCP

| Dia | Tarefas | Tempo Estimado | Prioridade |
|-----|---------|----------------|------------|
| **Seg 24/11** | • handle_unregister() (servidor)<br>• cmd_unregister() (cliente) | 2h | 🟡 Média |
| **Ter 25/11** | • Funções de gestão de eventos em database<br>• create_event(), get_event() | 3h | 🔴 Alta |
| **Qua 26/11** | • handle_myevents() (UDP servidor)<br>• cmd_my_events() (cliente) | 3h | 🟡 Média |
| **Qui 27/11** | • Funções de reservas em database<br>• make_reservation(), list_user_reservations() | 3h | 🔴 Alta |
| **Sex 28/11** | • handle_myreservations() (UDP)<br>• cmd_my_reservations() (cliente) | 2h | 🟡 Média |
| **Sáb 29/11** | • Criar tcp_server_thread()<br>• handle_changepass() (TCP) | 4h | 🔴 Alta |
| **Dom 30/11** | • cmd_change_password() (cliente TCP)<br>• **TESTE MILESTONE 2** | 3h | 🔴 Alta |

**✅ Milestone 2:** Todos comandos UDP funcionais + ChangePass TCP

**Comandos funcionais:**
```bash
> login 123456 pass1234
> myevents
No events created.
> myr
No reservations made.
> changePass pass1234 newpass1
Password changed successfully.
> unregister
User unregistered.
```

---

### **SEMANA 3: 1-7 Dezembro** 📁
**Objetivo:** Transfer de Ficheiros

| Dia | Tarefas | Tempo Estimado | Prioridade |
|-----|---------|----------------|------------|
| **Seg 01/12** | • Implementar `file_transfer.c`<br>• send_file_tcp() e receive_file_tcp() | 4h | 🔴 Alta |
| **Ter 02/12** | • handle_create() no servidor<br>• Receber e guardar ficheiros | 4h | 🔴 Alta |
| **Qua 03/12** | • cmd_create_event() no cliente<br>• Enviar ficheiros | 3h | 🔴 Alta |
| **Qui 04/12** | • Testes com ficheiros grandes (5-10MB)<br>• handle_show() no servidor | 4h | 🔴 Alta |
| **Sex 05/12** | • cmd_show_event() no cliente<br>• Download de ficheiros | 3h | 🔴 Alta |
| **Sáb 06/12** | • handle_list() (TCP servidor)<br>• cmd_list_events() (cliente) | 3h | 🟡 Média |
| **Dom 07/12** | • **TESTE MILESTONE 3**<br>• Criar, listar e mostrar eventos | 3h | 🔴 Alta |

**✅ Milestone 3:** Eventos com ficheiros funcionais

**Comandos funcionais:**
```bash
> create MyConcert poster.jpg 25-12-2025 100
Event created successfully. EID: 001
> list
001 MyConcert 1 25-12-2025
> show 001
Event: MyConcert
Date: 25-12-2025
Total seats: 100
Reserved: 0
File saved: poster.jpg
```

---

### **SEMANA 4: 8-14 Dezembro** 🎫
**Objetivo:** Sistema de Reservas Completo

| Dia | Tarefas | Tempo Estimado | Prioridade |
|-----|---------|----------------|------------|
| **Seg 08/12** | • handle_reserve() no servidor<br>• Lógica de validação de lugares | 4h | 🔴 Alta |
| **Ter 09/12** | • cmd_reserve() no cliente<br>• Testes de reservas normais | 3h | 🔴 Alta |
| **Qua 10/12** | • Testar casos limite:<br>  - Overbooking<br>  - Evento fechado<br>  - Evento passado | 4h | 🔴 Alta |
| **Qui 11/12** | • handle_close() no servidor<br>• Verificação de estados de eventos | 3h | 🔴 Alta |
| **Sex 12/12** | • cmd_close_event() no cliente<br>• Testes de fecho | 2h | 🟡 Média |
| **Sáb 13/12** | • Atualização automática de estados<br>• Sold-out automático | 3h | 🟡 Média |
| **Dom 14/12** | • **TESTE MILESTONE 4**<br>• Fluxo completo com 2+ clientes | 4h | 🔴 Alta |

**✅ Milestone 4:** Sistema completo funcional

**Teste com 2 clientes:**
```bash
# Terminal 1 - User A
> login 111111 pass1111
> create Concert concert.jpg 20-12-2025 10
Event created. EID: 001
> myevents
001 1 (0/10 reserved)

# Terminal 2 - User B
> login 222222 pass2222
> list
001 Concert 1 20-12-2025
> reserve 001 5
Reservation accepted.
> myr
001 20-12-2025 5

# Terminal 1
> myevents
001 1 (5/10 reserved)
> close 001
Event closed successfully.

# Terminal 2
> reserve 001 2
Event is closed.
```

---

### **SEMANA 5: 15-19 Dezembro** 🏁
**Objetivo:** Finalização e Entrega

| Dia | Tarefas | Tempo Estimado | Prioridade |
|-----|---------|----------------|------------|
| **Seg 15/12** | • Implementar persistência em ficheiros<br>• Salvar/Carregar users, events, reservations | 5h | 🔴 Alta |
| **Ter 16/12** | • Tratamento robusto de erros<br>• Validação completa de protocolos<br>• Valgrind (memory leaks) | 4h | 🔴 Alta |
| **Qua 17/12** | • Testes com múltiplos clientes (5+)<br>• Verificar sincronização (mutexes)<br>• Performance tests | 4h | 🔴 Alta |
| **Qui 18/12** | • Code review completo<br>• Adicionar comentários<br>• Atualizar README.md<br>• Preparar ficheiros de teste | 4h | 🔴 Alta |
| **Sex 19/12** | • **Testes finais completos**<br>• Criar projeto.zip<br>• Preencher auto-avaliação<br>• **SUBMETER até 23:59** 🎯 | 5h | 🔴 CRÍTICA |

**✅ Milestone Final:** Projeto completo e testado

---

## 📂 Estrutura do Projeto

```
ES-Event-Reservation/
│
├── src/
│   ├── server/
│   │   ├── es_server.c          # Main do servidor
│   │   ├── es_udp.c             # Thread UDP + handlers
│   │   ├── es_tcp.c             # Thread TCP + handlers
│   │   └── es_database.c        # Gestão de dados
│   │
│   ├── client/
│   │   ├── user_client.c        # Main do cliente
│   │   ├── user_commands.c      # Implementação dos comandos
│   │   └── user_interface.c     # Interface CLI
│   │
│   └── common/
│       ├── protocol.c           # Protocolo UDP/TCP
│       ├── utils.c              # Validações e utilitários
│       └── file_transfer.c      # Transfer de ficheiros
│
├── include/
│   ├── es_server.h
│   ├── user_client.h
│   ├── protocol.h
│   └── utils.h
│
├── data/
│   ├── users/
│   │   └── users.dat
│   ├── events/
│   │   ├── events.dat
│   │   ├── reservations.dat
│   │   └── files/
│   │       ├── 001_poster.jpg
│   │       └── 002_info.pdf
│
├── tests/
│   ├── test_files/
│   │   ├── test_1kb.txt
│   │   ├── test_1mb.jpg
│   │   └── test_10mb.pdf
│   └── test_script.sh
│
├── docs/
│   ├── GUIDE.md                 # Este ficheiro
│   ├── ARCHITECTURE.md
│   └── PROTOCOL.md
│
├── Makefile
├── README.md
├── .gitignore
└── auto-avaliacao.xlsx
```

## 🔌 Protocolo de Comunicação

### **UDP Protocol (Comandos Rápidos)**

| Comando | Request | Response | Uso |
|---------|---------|----------|-----|
| **Login** | `LIN UID password\n` | `RLI OK\|NOK\|REG\|ERR\n` | Autenticar/registar |
| **Logout** | `LOU UID password\n` | `RLO OK\|NOK\|UNR\|WRP\n` | Terminar sessão |
| **Unregister** | `UNR UID password\n` | `RUR OK\|NOK\|UNR\|WRP\n` | Remover conta |
| **My Events** | `LME UID password\n` | `RME OK [EID state]* \|NOK\|NLG\|WRP\n` | Listar meus eventos |
| **My Reservations** | `LMR UID password\n` | `RMR OK [EID date value]* \|NOK\|NLG\|WRP\n` | Listar minhas reservas |

### **TCP Protocol (Transfer de Dados)**

| Comando | Request | Response | Uso |
|---------|---------|----------|-----|
| **Create Event** | `CRE UID pass name date size Fname Fsize Fdata\n` | `RCE OK EID\|NOK\|NLG\|WRP\|ERR\n` | Criar evento |
| **Close Event** | `CLS UID password EID\n` | `RCL OK\|NOK\|NOE\|EOW\|SLD\|PST\|CLO\|NLG\n` | Fechar evento |
| **List Events** | `LST\n` | `RLS OK [EID name state date]* \|NOK\n` | Listar todos |
| **Show Event** | `SED EID\n` | `RSE OK UID name date total reserved Fname Fsize Fdata\|NOK\n` | Ver detalhes |
| **Reserve** | `RID UID pass EID people\n` | `RRI ACC\|REJ n\|CLS\|SLD\|PST\|NOK\|NLG\|WRP\n` | Fazer reserva |
| **Change Password** | `CPS UID oldPass newPass\n` | `RCP OK\|NOK\|NLG\|NID\n` | Alterar password |

### **Status Codes**

| Code | Significado | Contexto |
|------|-------------|----------|
| `OK` | Sucesso | Qualquer operação |
| `NOK` | Falha genérica | Operação não completada |
| `ERR` | Erro de sintaxe/protocolo | Mensagem inválida |
| `REG` | Novo utilizador registado | Login de novo user |
| `UNR` | User não registado | User não existe |
| `NLG` | Not logged in | User não está logged in |
| `WRP` | Wrong password | Password incorreta |
| `NOE` | No such event | EID não existe |
| `EOW` | Event owned by other | Não é owner do evento |
| `SLD` | Sold out | Evento esgotado |
| `PST` | Past event | Data já passou |
| `CLO` | Closed | Evento fechado |
| `ACC` | Accepted | Reserva aceite |
| `REJ` | Rejected | Reserva rejeitada |
| `CLS` | Closed | Evento não aceita reservas |
| `NID` | No such ID | UID não existe |

### **Event States**

| State | Valor | Significado |
|-------|-------|-------------|
| Active | `1` | Evento futuro, aceita reservas |
| Past | `0` | Data já passou |
| Sold Out | `2` | Esgotado (todos lugares reservados) |
| Closed | `3` | Fechado pelo owner |

---

## ✅ Checklist de Implementação

### **Servidor (ES)**

#### Setup Básico
- [ ] Criar socket UDP (bind em ESport)
- [ ] Criar socket TCP (bind em ESport, listen)
- [ ] Thread UDP (loop infinito, recvfrom)
- [ ] Thread TCP (loop infinito, accept + nova thread por cliente)
- [ ] Verbose mode (-v flag)
- [ ] Signal handling (CTRL+C graceful shutdown)

#### Database (es_database.c)
- [ ] Array de Users com mutex
- [ ] Array de Events com mutex
- [ ] Array de Reservations com mutex
- [ ] `register_user(uid, password)` → bool
- [ ] `authenticate_user(uid, password)` → bool
- [ ] `login_user(uid)` → bool
- [ ] `logout_user(uid)` → bool
- [ ] `is_user_logged_in(uid)` → bool
- [ ] `user_exists(uid)` → bool
- [ ] `change_password(uid, old, new)` → bool
- [ ] `unregister_user(uid)` → bool
- [ ] `create_event(uid, name, date, seats, filename, data, size)` → int (EID)
- [ ] `get_event(eid)` → Event*
- [ ] `close_event(uid, eid)` → status
- [ ] `list_all_events(count)` → Event**
- [ ] `list_user_events(uid, count)` → Event**
- [ ] `make_reservation(uid, eid, num_people, available)` → bool
- [ ] `list_user_reservations(uid, count)` → Reservation**
- [ ] `update_event_states()` (verificar datas, sold-out)
- [ ] `save_users_to_file()`
- [ ] `load_users_from_file()`
- [ ] `save_events_to_file()`
- [ ] `load_events_from_file()`
- [ ] `save_reservations_to_file()`
- [ ] `load_reservations_from_file()`

#### UDP Handlers (es_udp.c)
- [ ] `handle_login()` - LIN → RLI
- [ ] `handle_logout()` - LOU → RLO
- [ ] `handle_unregister()` - UNR → RUR
- [ ] `handle_myevents()` - LME → RME
- [ ] `handle_myreservations()` - LMR → RMR

#### TCP Handlers (es_tcp.c)
- [ ] `handle_create()` - CRE → RCE + receber ficheiro
- [ ] `handle_close()` - CLS → RCL
- [ ] `handle_list()` - LST → RLS
- [ ] `handle_show()` - SED → RSE + enviar ficheiro
- [ ] `handle_reserve()` - RID → RRI
- [ ] `handle_changepass()` - CPS → RCP

### **Cliente (User)**

#### Setup Básico
- [ ] Criar socket UDP (para comunicação com ES)
- [ ] Parse argumentos (-n ESIP, -p ESport)
- [ ] Loop de comandos (ler stdin)
- [ ] ClientState (uid, password, logged_in, server_ip, server_port)

#### Comandos UDP (user_commands.c)
- [ ] `cmd_login(uid, password)` - LIN → RLI
- [ ] `cmd_logout()` - LOU → RLO
- [ ] `cmd_unregister()` - UNR → RUR
- [ ] `cmd_my_events()` - LME → RME
- [ ] `cmd_my_reservations()` - LMR → RMR

#### Comandos TCP (user_commands.c)
- [ ] `cmd_create_event(name, filename, date, seats)` - CRE → RCE + enviar ficheiro
- [ ] `cmd_close_event(eid)` - CLS → RCL
- [ ] `cmd_list_events()` - LST → RLS
- [ ] `cmd_show_event(eid)` - SED → RSE + receber ficheiro
- [ ] `cmd_reserve(eid, num_people)` - RID → RRI
- [ ] `cmd_change_password(old, new)` - CPS → RCP

#### Interface (user_interface.c)
- [ ] `parse_command(input)` - parsear comando e chamar função adequada
- [ ] `display_help()` - mostrar comandos disponíveis
- [ ] `display_events(events)` - mostrar lista de eventos formatada
- [ ] `display_reservations(reservations)` - mostrar lista de reservas

### **Módulos Comuns**

#### protocol.c
- [ ] `send_udp_message(sockfd, message, addr, addrlen)`
- [ ] `receive_udp_message(sockfd, buffer, size, addr, addrlen)`
- [ ] `send_tcp_message(sockfd, message)`
- [ ] `receive_tcp_message(sockfd, buffer, size)`
- [ ] Tratar partial reads/writes

#### file_transfer.c
- [ ] `send_file_tcp(sockfd, filename, filesize)`
- [ ] `receive_file_tcp(sockfd, filename, filesize)`
- [ ] Enviar/receber em chunks (1024 bytes)
- [ ] Validar checksum (opcional)

#### utils.c
- [ ] `validate_uid(uid)` - 6 dígitos
- [ ] `validate_password(password)` - 8 alfanuméricos
- [ ] `validate_event_name(name)` - max 10 alfanuméricos
- [ ] `validate_date(date)` - formato dd-mm-yyyy
- [ ] `validate_filename(filename)` - max 24 chars, extensão válida
- [ ] `get_current_date()` - retornar data atual
- [ ] `compare_dates(date1, date2)` - comparar datas
- [ ] `safe_read(fd, buffer, size)` - ler até size bytes
- [ ] `safe_write(fd, buffer, size)` - escrever todos bytes

---

## 🧪 Testes e Validação

### **Teste 1: Login/Logout (UDP)**
```bash
# Terminal 1 - Servidor
./ES -p 58001 -v

# Terminal 2 - Cliente
./user -n 127.0.0.1 -p 58001
> login 123456 pass1234
New user registered and logged in.
> logout
Logout successful.
> login 123456 wrongpass
Incorrect password.
> login 123456 pass1234
Login successful.
> logout
Logout successful.
```

### **Teste 2: Criar e Listar Eventos**
```bash
# Criar ficheiro de teste
echo "Concert info" > concert.txt

> login 111111 pass1111
> create MyConcert concert.txt 25-12-2025 100
Event created successfully. EID: 001
> myevents
001 1
> list
001 MyConcert 1 25-12-2025
> show 001
Owner: 111111
Event: MyConcert
Date: 25-12-2025
Total seats: 100
Reserved: 0
File saved: concert.txt
```

### **Teste 3: Reservas Multi-Cliente**
```bash
# Terminal 1 - User A (Owner)
> login 111111 pass1111
> create Party party.jpg 31-12-2025 20
Event created. EID: 001

# Terminal 2 - User B
> login 222222 pass2222
> list
001 Party 1 31-12-2025
> reserve 001 5
Reservation accepted.

# Terminal 3 - User C
> login 333333 pass3333
> reserve 001 10
Reservation accepted.

# Terminal 2
> reserve 001 10
Reservation rejected. Only 5 seats available.

# Terminal 1
> myevents
001 1 (15/20 reserved)
> close 001
Event closed successfully.

# Terminal 3
> reserve 001 1
Event is closed.
```

### **Teste 4: Concorrência (Race Conditions)**
```bash
# Script de teste automático
#!/bin/bash

# Iniciar servidor
./ES -p 58001 &
SERVER_PID=$!
sleep 1

# Criar evento com 10 lugares
echo "login 111111 pass1111" | ./user -n 127.0.0.1 -p 58001
echo "create Test test.txt 31-12-2025 10" | ./user -n 127.0.0.1 -p 58001

# Tentar reservar 10 lugares simultaneamente (20 clientes, 1 lugar cada)
for i in {1..20}; do
    (echo "login user${i} pass${i}"; echo "reserve 001 1") | \
    ./user -n 127.0.0.1 -p 58001 &
done
wait

# Verificar: apenas 10 devem ter sucesso
echo "myevents" | ./user -n 127.0.0.1 -p 58001 | grep "001"
# Deve mostrar: 001 1 (10/10 reserved) ou 001 2 (sold-out)

kill $SERVER_PID
```

### **Teste 5: Ficheiros Grandes (10MB)**
```bash
# Criar ficheiro de 10MB
dd if=/dev/urandom of=big_file.bin bs=1M count=10

> login 111111 pass1111
> create BigEvent big_file.bin 01-01-2026 50
Event created. EID: 001

# Verificar tamanho do ficheiro guardado no servidor
ls -lh data/events/files/001_big_file.bin
# Deve ser ~10MB

> show 001
# Deve fazer download correto
ls -lh big_file.bin
```

### **Teste 6: Persistência**
```bash
# Terminal 1
./ES -p 58001
> login 111111 pass1111
> create Event1 file.txt 25-12-2025 100
Event created. EID: 001
> (Pressionar CTRL+C para parar servidor)

# Reiniciar servidor
./ES -p 58001

# Terminal 2
> login 111111 pass1111
Login successful.  # User deve existir
> myevents
001 1  # Evento deve estar presente
```

### **Teste 7: Validações e Erros**
```bash
> login 12345 pass1234
Invalid UID. Must be 6 digits.

> login 123456 pass123
Invalid password. Must be 8 alphanumeric characters.

> create ThisIsAVeryLongEventName file.txt 25-12-2025 100
Invalid event name. Maximum 10 characters.

> create Event file.txt 32-13-2025 100
Invalid date format.

> create Event file.txt 25-12-2025 5
Invalid attendance size. Must be between 10 and 999.

> reserve 999 10
Event does not exist.

> reserve 001 -5
Invalid number of seats.

> reserve 001 1000
Invalid number of seats. Must be between 1 and 999.
```

### **Teste 8: Estados de Eventos**
```bash
# Criar evento no passado (manualmente editar data no servidor)
# ou esperar data passar (não viável em teste)

# Simular: modificar get_current_date() para retornar data futura

> login 111111 pass1111
> create OldEvent old.txt 15-11-2025 50
> myevents
001 0  # Estado = Past (0)

> close 001
Event date has already passed.

> login 222222 pass2222
> reserve 001 5
Event date has already passed.
```

---

## 📦 Submissão

### **Preparar Submissão**

```bash
# 1. Criar diretório limpo
mkdir proj_GN_submission
cd proj_GN_submission

# 2. Copiar ficheiros necessários
cp -r src include Makefile README.md ../ES-Event-Reservation/

# 3. Criar ficheiro auto-avaliacao.xlsx
# (preencher no Excel)

# 4. Testar compilação limpa
make clean
make

# 5. Testar executáveis
./ES -p 58001 &
./user -n 127.0.0.1 -p 58001
# (fazer alguns testes)

# 6. Criar zip
cd ..
zip -r proj_GN.zip proj_GN_submission/

# 7. Verificar conteúdo do zip
unzip -l proj_GN.zip
```

### **Checklist Final**

- [ ] `make clean && make` compila sem erros/warnings
- [ ] `./ES -p 58001` inicia servidor
- [ ] `./user -n 127.0.0.1 -p 58001` conecta ao servidor
- [ ] Todos os comandos UDP funcionam
- [ ] Todos os comandos TCP funcionam
- [ ] Transfer de ficheiros funciona (< 10MB)
- [ ] Múltiplos clientes simultâneos
- [ ] Sem race conditions (testar com script)
- [ ] Persistência funciona (reiniciar servidor)
- [ ] Tratamento de erros completo
- [ ] Código comentado adequadamente
- [ ] README.md com instruções claras
- [ ] auto-avaliacao.xlsx preenchida
- [ ] Ficheiros desnecessários removidos (.o, executáveis antigos)
- [ ] Estrutura do zip correta
- [ ] Tamanho do zip razoável (< 100KB sem ficheiros de teste)

### **Conteúdo do README.md**

```markdown
# Event Reservation System

## Grupo XX

- Aluno 1: Nome (istXXXXXX)
- Aluno 2: Nome (istYYYYYY)

## Compilação

```bash
make
```

## Execução

### Servidor
```bash
./ES [-p ESport] [-v]
```
- `-p ESport`: Porta do servidor (default: 58001)
- `-v`: Modo verbose

### Cliente
```bash
./user [-n ESIP] [-p ESport]
```
- `-n ESIP`: IP do servidor (default: 127.0.0.1)
- `-p ESport`: Porta do servidor (default: 58001)

## Comandos Disponíveis

### Gestão de Conta
- `login UID password` - Login/Registo
- `logout` - Logout
- `unregister` - Remover conta
- `changePass oldPass newPass` - Alterar password

### Gestão de Eventos
- `create name file date seats` - Criar evento
- `close EID` - Fechar evento
- `myevents` ou `mye` - Listar meus eventos

### Reservas
- `list` - Listar todos os eventos
- `show EID` - Ver detalhes de evento
- `reserve EID num` - Reservar lugares
- `myreservations` ou `myr` - Listar minhas reservas

### Outros
- `exit` - Sair (após logout)

## Estrutura do Projeto

```
src/
├── server/     - Código do servidor
├── client/     - Código do cliente
└── common/     - Código partilhado
include/        - Headers
data/           - Dados persistentes (criado em runtime)
```

## Notas de Implementação

- Protocolo UDP para comandos rápidos
- Protocolo TCP para transfer de ficheiros
- Mutexes para sincronização de acesso a dados partilhados
- Persistência em ficheiros binários
- Suporta ficheiros até 10MB
- Máximo 1000 utilizadores, 999 eventos

## Testes Realizados

- Login/Logout múltiplos utilizadores
- Criação de eventos com ficheiros grandes
- Reservas concorrentes (race conditions)
- Persistência de dados
- Validação de inputs
```

### **Email de Submissão**

```
Para: [professor.lab@tecnico.ulisboa.pt]
Assunto: [RC] Projeto Grupo XX - Event Reservation

Corpo:
Exmo. Professor,

Segue em anexo o projeto do Grupo XX.

Ficheiro: proj_XX.zip
Tamanho: XX KB
MD5: [hash MD5 do ficheiro]

Alunos:
- Nome Aluno 1 (istXXXXXX)
- Nome Aluno 2 (istYYYYYY)

Cumprimentos,
Grupo XX
```

---

## 💡 Dicas Finais

### **Durante o Desenvolvimento**

1. **Commits frequentes no Git**
   ```bash
   git add .
   git commit -m "Implementar handle_login() em UDP"
   git push
   ```

2. **Testar incrementalmente**
   - Não implementar tudo de uma vez
   - Testar cada função individualmente
   - Só avançar quando funcionar

3. **Debugging**
   ```bash
   # Usar gdb
   gdb ./ES
   (gdb) run -p 58001 -v
   (gdb) break handle_login
   (gdb) continue
   
   # Usar valgrind para memory leaks
   valgrind --leak-check=full ./ES -p 58001
   ```

4. **Printf debugging no verbose mode**
   ```c
   if (verbose_mode) {
       printf("[DEBUG] Received: %s from %s:%d\n", 
              buffer, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
   }
   ```

### **Erros Comuns**

❌ **Problema:** `bind: Address already in use`
```bash
# Solução
lsof -i :58001
kill -9 [PID]
```

❌ **Problema:** Partial read/write em TCP
```c
// ❌ Errado
n = read(sockfd, buffer, 1024);

// ✅ Correto
while (total < size) {
    n = read(sockfd, buffer + total, size - total);
    if (n <= 0) break;
    total += n;
}
```

❌ **Problema:** Race condition em reservas
```c
// ❌ Errado (sem mutex)
if (event.available_seats >= num_seats) {
    event.available_seats -= num_seats;  // RACE!
}

// ✅ Correto (com mutex)
pthread_mutex_lock(&events_mutex);
if (event.available_seats >= num_seats) {
    event.available_seats -= num_seats;
}
pthread_mutex_unlock(&events_mutex);
```

❌ **Problema:** Buffer overflow
```c
// ❌ Errado
char buffer[64];
sprintf(buffer, "Very long message..."); // Overflow!

// ✅ Correto
char buffer[256];
snprintf(buffer, sizeof(buffer), "Message: %s", msg);
```

### **Performance Tips**

1. **Usar `select()` para multiplexing** (opcional, avançado)
2. **Buffer size adequado** (1024-4096 bytes para transfer de ficheiros)
3. **Evitar chamadas de sistema desnecessárias**
4. **Limpar buffers antes de usar** (`memset()`)

---

## 🎓 Discussão do Projeto

### Perguntas Esperadas

1. **Arquitetura**
   - "Porque escolheste UDP para login e TCP para create?"
   - "Como funciona a sincronização entre threads?"

2. **Implementação**
   - "Como garantes que não há overbooking?"
   - "Como tratas desconexões inesperadas?"
   - "Como persistes os dados?"

3. **Testes**
   - "Como testaste concorrência?"
   - "Qual foi o maior desafio?"

4. **Melhorias**
   - "Como poderias melhorar a segurança?"
   - "Como escalarias para milhares de eventos?"

### Preparação

- [ ] Rever todo o código
- [ ] Entender cada função
- [ ] Saber explicar decisões de design
- [ ] Ter exemplos de testes prontos
- [ ] Conhecer limitações do sistema

---

## 📚 Recursos Úteis

- **Man pages:** `man socket`, `man pthread`, `man select`
- **Beej's Guide to Network Programming:** https://beej.us/guide/bgnet/
- **POSIX Threads:** https://www.cs.cmu.edu/afs/cs/academic/class/15492-f07/www/pthreads.html
- **Git Tutorial:** https://git-scm.com/docs/gittutorial

---

**Boa sorte! 🚀**

*Último update: 17 Nov 2025*