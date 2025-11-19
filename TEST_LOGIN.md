# Teste do comando LOGIN

## Como executar

### Terminal 1 - Servidor
```bash
./ES
```

### Terminal 2 - Cliente
```bash
./user
```

## Cenários de teste

### 1. Novo utilizador (primeiro login - REG)
```
> login 123456 Pass1234
```
**Resposta esperada:** `New user registered`

### 2. Utilizador existente (login correto - OK)
```
> login 123456 Pass1234
```
**Resposta esperada:** `Login successful`

### 3. Password incorreta (NOK)
```
> login 123456 WrongPwd
```
**Resposta esperada:** `Incorrect login attempt`

### 4. Formato inválido UID (ERR)
```
> login 12345 Pass1234
```
**Resposta esperada:** `Error: UID must be exactly 6 digits`

### 5. Formato inválido password (ERR)
```
> login 123456 Pass123
```
**Resposta esperada:** `Error: Password must be exactly 8 alphanumeric characters`

## Verificar dados persistidos

```bash
ls -la USERS/
cat USERS/123456_pass.txt
```

## Comandos disponíveis no cliente

- `login UID password` - Fazer login
- `help` - Mostrar ajuda
- `exit` - Sair
