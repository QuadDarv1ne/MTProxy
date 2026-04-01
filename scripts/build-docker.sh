#!/bin/bash
# Скрипт сборки Docker образа MTProxy

set -e

IMAGE_NAME="mtproxy"
IMAGE_TAG="latest"

echo "=== MTProxy Docker Build ==="
echo "Image: ${IMAGE_NAME}:${IMAGE_TAG}"

# Сборка образа
docker build -t ${IMAGE_NAME}:${IMAGE_TAG} .

if [ $? -eq 0 ]; then
    echo "✓ Образ успешно собран"
    echo ""
    echo "Запуск:"
    echo "  docker run -d --name mtproxy -p 8080:8080 ${IMAGE_NAME}:${IMAGE_TAG}"
    echo ""
    echo "Или используйте docker-compose:"
    echo "  docker-compose up -d"
else
    echo "✗ Ошибка сборки"
    exit 1
fi
