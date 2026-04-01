#!/bin/bash
# Скрипт развёртывания MTProxy в Docker

set -e

echo "=== MTProxy Docker Deploy ==="

# Проверка docker-compose
if ! command -v docker-compose &> /dev/null; then
    echo "✗ docker-compose не найден"
    exit 1
fi

# Создание директории конфигурации
mkdir -p config

# Генерация конфигурации если не существует
if [ ! -f config/proxy.json ]; then
    echo "Генерация конфигурации..."
    cat > config/proxy.json << 'EOF'
{
    "proxy": {
        "port": 8080,
        "secret": "CHANGE_ME_SECRET",
        "workers": 4,
        "max_connections": 10000
    },
    "logging": {
        "level": "info",
        "file": "/var/log/mtproxy/proxy.log"
    }
}
EOF
    echo "✓ Конфигурация создана: config/proxy.json"
    echo "  Измените secret на случайное значение!"
fi

# Запуск
echo "Запуск MTProxy..."
docker-compose up -d

echo ""
echo "✓ MTProxy запущен"
echo ""
echo "Статус:"
echo "  docker-compose ps"
echo ""
echo "Логи:"
echo "  docker-compose logs -f mtproxy"
echo ""
echo "Остановка:"
echo "  docker-compose down"
