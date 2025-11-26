# Migração para SQLite - Guia Completo

## 🎯 O que muda?

**NADA na lógica!** Só a implementação interna:

- `user_management.c` (ficheiros) → `user_database.c` (SQLite)
- API pública mantém-se **exatamente igual**
- `register_user()`, `authenticate_user()` funcionam da mesma forma

## 📦 Pré-requisitos

Instalar SQLite3:

```bash
# Ubuntu/Debian
sudo apt-get install libsqlite3-dev

# MacOS
brew install sqlite3

# Verificar instalação
sqlite3 --version
```

## 🔄 Como usar SQLite

### Opção A: Compilar com SQLite (novo projeto)

```bash
# Compilar com SQLite
make -f Makefile.sqlite clean
make -f Makefile.sqlite

# Executar
./ES
./user
```

### Opção B: Migrar dados existentes

Se já tens utilizadores em `USERS/`:

```bash
# 1. Compilar versão SQLite
make -f Makefile.sqlite clean
make -f Makefile.sqlite

# 2. Migrar dados automaticamente
make -f Makefile.sqlite migrate

# Output esperado:
# Migrating users from USERS/ to SQLite...
#   Migrated user: 123456
#   Migrated user: 999888
# Migration complete!

# 3. Executar servidor
./ES
```

## 📊 Verificar dados

### Ver utilizadores na base de dados:

```bash
# Via Makefile
make -f Makefile.sqlite listusers

# Ou diretamente
sqlite3 es_server.db "SELECT * FROM users;"
```

### Entrar no SQLite interativo:

```bash
sqlite3 es_server.db

# Comandos úteis:
.tables                              # Listar tabelas
.schema users                        # Ver estrutura da tabela
SELECT * FROM users;                 # Ver todos os utilizadores
SELECT COUNT(*) FROM users;          # Contar utilizadores
SELECT * FROM users WHERE uid='123456';  # Buscar específico
.quit                               # Sair
```

## 🔍 Comparação

### Antes (Ficheiros)
```
USERS/
├── 123456_pass.txt  → "Pass1234"
├── 999888_pass.txt  → "Abc12345"
└── 777666_pass.txt  → "Secret99"
```

### Depois (SQLite)
```
es_server.db
┌────────┬──────────┬─────────────────────┐
│  uid   │ password │     created_at      │
├────────┼──────────┼─────────────────────┤
│ 123456 │ Pass1234 │ 2025-11-19 10:30:15 │
│ 999888 │ Abc12345 │ 2025-11-19 10:31:42 │
│ 777666 │ Secret99 │ 2025-11-19 10:32:08 │
└────────┴──────────┴─────────────────────┘
```

## 🧪 Testar

### 1. Teste básico (novo utilizador)
```bash
# Terminal 1
./ES

# Terminal 2
./user
> login 111111 TestPass
New user registered

# Verificar
sqlite3 es_server.db "SELECT * FROM users WHERE uid='111111';"
```

### 2. Teste após restart
```bash
# Terminal 1 - Parar servidor (Ctrl+C) e reiniciar
./ES

# Terminal 2
./user
> login 111111 TestPass
Login successful  ← Dados persistiram!
```

### 3. Teste password incorreta
```bash
> login 111111 WrongPwd
Incorrect login attempt
```

## 🔧 Comandos úteis

```bash
# Compilar versão SQLite
make -f Makefile.sqlite

# Limpar tudo
make -f Makefile.sqlite clean

# Apagar base de dados
make -f Makefile.sqlite cleandb

# Migrar dados USERS/ → SQLite
make -f Makefile.sqlite migrate

# Ver utilizadores
make -f Makefile.sqlite listusers

# Executar servidor
make -f Makefile.sqlite run_server

# Executar cliente
make -f Makefile.sqlite run_client
```

## 🆚 Alternar entre versões

### Usar Ficheiros (original):
```bash
make clean
make
./ES  # Usa USERS/
```

### Usar SQLite:
```bash
make -f Makefile.sqlite clean
make -f Makefile.sqlite
./ES  # Usa es_server.db
```

## 📈 Vantagens SQLite

✅ **Performance**: Mais rápido para múltiplos utilizadores  
✅ **Concorrência**: Suporta múltiplos acessos simultâneos  
✅ **Queries**: Podes fazer pesquisas complexas  
✅ **Integridade**: Transações ACID garantidas  
✅ **Escalabilidade**: Suporta milhões de utilizadores  
✅ **Organização**: Um ficheiro em vez de centenas  
✅ **Metadados**: Timestamp de criação automático  

## 🚨 Notas importantes

1. **Não mistures**: Escolhe ficheiros OU SQLite, não ambos ao mesmo tempo
2. **Backup**: `cp es_server.db es_server.db.backup` antes de testar
3. **Migração**: Use `make -f Makefile.sqlite migrate` se já tens dados
4. **Produção**: Adiciona hashing de passwords (SHA-256) no futuro

## 🐛 Troubleshooting

### "cannot open shared object file: libsqlite3.so.0"
```bash
# Instalar biblioteca
sudo apt-get install libsqlite3-0 libsqlite3-dev
```

### "table users already exists"
```bash
# Normal, a tabela já foi criada
# Se quiseres recomeçar:
make -f Makefile.sqlite cleandb
```

### "database is locked"
```bash
# Outro processo está a usar a DB
# Fechar todos os servidores e tentar novamente
pkill ES
./ES
```

## ✅ Verificação

Depois de compilar e executar:

```bash
# 1. Verificar ficheiro DB criado
ls -lh es_server.db

# 2. Ver estrutura
sqlite3 es_server.db ".schema"

# 3. Testar login
./user
> login 123456 Pass1234
New user registered

# 4. Confirmar na DB
sqlite3 es_server.db "SELECT * FROM users;"
```

---

**Pronto!** Agora tens duas versões:
- `Makefile` → Ficheiros (simples, para desenvolvimento)
- `Makefile.sqlite` → SQLite (robusto, para produção)

Escolhe qual usar conforme a necessidade! 🎉
