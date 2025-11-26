#!/bin/bash
# Query helper para debug da base de dados SQLite
# Uso: ./db_queries.sh [comando]

DB_FILE="es_server.db"

if [ ! -f "$DB_FILE" ]; then
    echo "❌ Base de dados não encontrada: $DB_FILE"
    echo "Execute o servidor primeiro para criar a BD"
    exit 1
fi

case "$1" in
    "users"|"list"|"")
        echo "=== TODOS OS UTILIZADORES ==="
        sqlite3 $DB_FILE "SELECT uid, password, created_at FROM users ORDER BY created_at DESC;"
        ;;
    
    "count")
        echo "=== TOTAL DE UTILIZADORES ==="
        sqlite3 $DB_FILE "SELECT COUNT(*) as total FROM users;"
        ;;
    
    "find")
        if [ -z "$2" ]; then
            echo "Uso: ./db_queries.sh find <UID>"
            exit 1
        fi
        echo "=== PROCURAR UTILIZADOR $2 ==="
        sqlite3 $DB_FILE "SELECT * FROM users WHERE uid='$2';"
        ;;
    
    "recent")
        echo "=== ÚLTIMOS 10 REGISTOS ==="
        sqlite3 $DB_FILE "SELECT uid, password, datetime(created_at, 'localtime') as created FROM users ORDER BY created_at DESC LIMIT 10;"
        ;;
    
    "delete")
        if [ -z "$2" ]; then
            echo "Uso: ./db_queries.sh delete <UID>"
            exit 1
        fi
        echo "A apagar utilizador $2..."
        sqlite3 $DB_FILE "DELETE FROM users WHERE uid='$2';"
        echo "✓ Utilizador $2 apagado"
        ;;
    
    "clear"|"reset")
        read -p "⚠️  Apagar TODOS os utilizadores? (yes/n): " confirm
        if [ "$confirm" = "yes" ]; then
            sqlite3 $DB_FILE "DELETE FROM users;"
            echo "✓ Todos os utilizadores apagados"
        else
            echo "Cancelado"
        fi
        ;;
    
    "schema")
        echo "=== ESTRUTURA DA TABELA ==="
        sqlite3 $DB_FILE ".schema users"
        ;;
    
    "sql")
        if [ -z "$2" ]; then
            echo "Uso: ./db_queries.sh sql 'SELECT ...'"
            exit 1
        fi
        echo "=== QUERY PERSONALIZADA ==="
        sqlite3 $DB_FILE "$2"
        ;;
    
    "interactive"|"shell")
        echo "=== MODO INTERATIVO ==="
        echo "Comandos úteis:"
        echo "  .tables          - Listar tabelas"
        echo "  .schema users    - Ver estrutura"
        echo "  SELECT * FROM users; - Ver utilizadores"
        echo "  .quit            - Sair"
        echo ""
        sqlite3 $DB_FILE
        ;;
    
    "check")
        echo "=== VERIFICAÇÃO DA BASE DE DADOS ==="
        echo "Ficheiro: $DB_FILE"
        echo "Tamanho: $(du -h $DB_FILE | cut -f1)"
        echo ""
        echo "Tabelas:"
        sqlite3 $DB_FILE ".tables"
        echo ""
        echo "Utilizadores:"
        sqlite3 $DB_FILE "SELECT COUNT(*) FROM users;"
        ;;
    
    "migrate")
        echo "=== MIGRAÇÃO DE USERS/ PARA SQLite ==="
        if [ ! -d "USERS" ]; then
            echo "❌ Diretório USERS/ não encontrado"
            exit 1
        fi
        
        count=0
        for file in USERS/*_pass.txt; do
            if [ -f "$file" ]; then
                uid=$(basename "$file" _pass.txt)
                password=$(cat "$file" | tr -d '\n')
                sqlite3 $DB_FILE "INSERT OR IGNORE INTO users (uid, password) VALUES ('$uid', '$password');"
                echo "  ✓ Migrado: $uid"
                ((count++))
            fi
        done
        echo ""
        echo "Total migrado: $count utilizadores"
        ;;
    
    "help"|"-h"|"--help")
        echo "=== DB QUERIES - COMANDOS DISPONÍVEIS ==="
        echo ""
        echo "Listar:"
        echo "  ./db_queries.sh users         - Listar todos os utilizadores"
        echo "  ./db_queries.sh count         - Contar utilizadores"
        echo "  ./db_queries.sh recent        - Últimos 10 registos"
        echo "  ./db_queries.sh find <UID>    - Procurar utilizador específico"
        echo ""
        echo "Modificar:"
        echo "  ./db_queries.sh delete <UID>  - Apagar utilizador"
        echo "  ./db_queries.sh clear         - Apagar TODOS (pede confirmação)"
        echo ""
        echo "Debug:"
        echo "  ./db_queries.sh schema        - Ver estrutura da tabela"
        echo "  ./db_queries.sh check         - Verificar estado da BD"
        echo "  ./db_queries.sh sql 'SELECT...' - Query personalizada"
        echo "  ./db_queries.sh interactive   - Modo interativo SQLite"
        echo ""
        echo "Migração:"
        echo "  ./db_queries.sh migrate       - Migrar USERS/ → SQLite"
        echo ""
        echo "Exemplos:"
        echo "  ./db_queries.sh users"
        echo "  ./db_queries.sh find 123456"
        echo "  ./db_queries.sh sql \"SELECT * FROM users WHERE uid LIKE '123%'\""
        ;;
    
    *)
        echo "❌ Comando desconhecido: $1"
        echo "Use: ./db_queries.sh help"
        exit 1
        ;;
esac
