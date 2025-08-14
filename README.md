# KVStorage

Простое key-value хранилище с поддержкой TTL, написанное на C++

## Возможности

- Быстрый доступ к данным по ключу (`get`, `set`, `remove`)
- TTL (время жизни) для каждой записи
- Получение отсортированных записей (`getManySorted`)
- Удаление одной просроченной записи (`removeOneExpiredEntry`)
- Написано с поддержкой тестирования (GoogleTest)

## Структура проекта

```bash

VK-CPP/
├── include/
│ └── kvstorage.hpp # Основная реализация
├── tests/
│ └── kvstorage_tests.cpp # Тесты с использованием GoogleTest
├── extern/
│ └── googletest/ # Подмодуль GoogleTest
├── CMakeLists.txt
└── README.md
```

## Как собрать

### 1. Клонировать проект с подмодулями

```bash
git clone --recursive https://github.com/your-name/VK-CPP.git
cd VK-CPP
```

### 1. Сборка через CMake

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### 3. Запуск тестов

```bash
./kvstorage_tests
```

или с помощью ctest

```bash
ctest -V
```

## Примечания

- Используется комбинация std::map и unordered_map для ускорения доступа и поддержки сортировки.

- TTL обрабатывается через std::chrono::time_point.