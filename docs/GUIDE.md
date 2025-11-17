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

---