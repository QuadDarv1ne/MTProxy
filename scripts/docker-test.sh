#!/bin/bash
#
# Docker Test Runner for MTProxy
# Скрипт для запуска Docker интеграционных тестов
#
# Использование:
#   ./scripts/docker-test.sh [build|test|clean|all]
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
DOCKER_IMAGE="mtproxy:latest"
CONTAINER_NAME="mtproxy-test"

# Цвета для вывода
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[PASS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[FAIL]${NC} $1"
}

# Проверка наличия Docker
check_docker() {
    if ! command -v docker &> /dev/null; then
        log_error "Docker not found. Please install Docker first."
        exit 1
    fi

    if ! docker info &> /dev/null; then
        log_error "Docker daemon is not running."
        exit 1
    fi

    log_success "Docker is available"
}

# Сборка Docker образа
build_image() {
    log_info "Building Docker image: $DOCKER_IMAGE"

    cd "$PROJECT_ROOT"

    if docker build -t "$DOCKER_IMAGE" . 2>&1 | tee /tmp/docker-build.log; then
        log_success "Docker image built successfully"

        # Показываем размер образа
        SIZE=$(docker images "$DOCKER_IMAGE" --format "{{.Size}}")
        log_info "Image size: $SIZE"
    else
        log_error "Failed to build Docker image"
        exit 1
    fi
}

# Запуск интеграционных тестов
run_tests() {
    log_info "Running Docker integration tests"

    cd "$PROJECT_ROOT/testing"

    if python3 test_docker_integration.py; then
        log_success "All Docker tests passed"
        return 0
    else
        log_error "Some Docker tests failed"
        return 1
    fi
}

# Очистка тестовых контейнеров
cleanup() {
    log_info "Cleaning up test containers"

    docker stop "$CONTAINER_NAME" 2>/dev/null || true
    docker rm "$CONTAINER_NAME" 2>/dev/null || true

    # Очистка висячих образов
    docker image prune -f 2>/dev/null || true

    log_success "Cleanup completed"
}

# Запуск всех тестов
run_all() {
    log_info "Running full Docker test suite"

    check_docker
    build_image
    run_tests
    cleanup

    log_success "All tests completed successfully"
}

# Показ справки
show_help() {
    echo "Docker Test Runner for MTProxy"
    echo ""
    echo "Usage: $0 [command]"
    echo ""
    echo "Commands:"
    echo "  build    Build Docker image"
    echo "  test     Run integration tests"
    echo "  clean    Cleanup test containers"
    echo "  all      Build, test, and cleanup (default)"
    echo "  help     Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0 build     # Only build the image"
    echo "  $0 test      # Only run tests"
    echo "  $0 all       # Full test suite"
}

# Main
main() {
    local command="${1:-all}"

    case "$command" in
        build)
            check_docker
            build_image
            ;;
        test)
            check_docker
            run_tests
            ;;
        clean)
            cleanup
            ;;
        all)
            run_all
            ;;
        help|--help|-h)
            show_help
            ;;
        *)
            log_error "Unknown command: $command"
            show_help
            exit 1
            ;;
    esac
}

main "$@"
