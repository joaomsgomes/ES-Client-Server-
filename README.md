# Event Reservation System

Sistema de reserva de eventos com comunicação cliente-servidor usando sockets UDP/TCP.

## Compilação

```bash
make
```

## Execução

### Servidor
```bash
./ES [-p ESport] [-v]
```

### Cliente
```bash
./user [-n ESIP] [-p ESport]
```

## Comandos Disponíveis

- `login UID password` - Login/Registo
- `logout` - Logout
- `unregister` - Remover conta
- `changePass old new` - Alterar password
- `create name file date seats` - Criar evento
- `close EID` - Fechar evento
- `list` - Listar todos os eventos
- `myevents` ou `mye` - Listar meus eventos
- `show EID` - Ver detalhes do evento
- `reserve EID value` - Reservar lugares
- `myreservations` ou `myr` - Listar minhas reservas
- `exit` - Sair

## Estrutura de Dados

Os dados são guardados em:
- `data/users/` - Informação dos utilizadores
- `data/events/` - Eventos e ficheiros associados