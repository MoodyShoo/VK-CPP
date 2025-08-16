# KVStorage

Простое key-value хранилище с поддержкой TTL, написанное на C++

## Возможности

- Быстрый доступ к данным по ключу (`get`, `set`, `remove`)
- TTL (время жизни) для каждой записи
- Получение отсортированных записей (`getManySorted`)
- Удаление одной просроченной записи (`removeOneExpiredEntry`)
- Написано с поддержкой тестирования (GoogleTest)
- Производительность проверена на миллион записей

## Структура проекта

```bash

VK-CPP/
├── include/
│ └── kvstorage.hpp # Основная реализация
├── tests/
│ └── kvstorage_tests.cpp # Тесты с использованием GoogleTest
│ └── kvstorage_perf_test.cpp 
├── extern/
│ └── googletest/ # Подмодуль GoogleTest
├── CMakeLists.txt
└── README.md
```

## Асимптотика

|Операция|Описание|Асимптотика|
|-|-|-|
| **Конструктор** | Инициализация из `n` записей | O(n log n) |
| **set(key, value, ttl)** | Вставка или обновление записи | O(log n)  |
| **remove(key)** | Удаление записи по ключу | O(1)* |
| **get(key)** | Получение значения по ключу | O(1)* |
| **getManySorted(key, count)**| Получение `count` записей с ключей ≥ `key` в лексикографическом порядке | O(log n + count) |
| **removeOneExpiredEntry()**  | Поиск и удаление одной протухшей записи | O(n) |

_\* O(1) амортизированное — благодаря хеш-таблице_

## Как собрать

### 1. Клонировать проект с подмодулями

```bash
git clone --recursive https://github.com/your-name/VK-CPP.git
cd VK-CPP
```

### 1. Сборка через CMake

```powershell
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

```powershell
ctest -V
```

- Тесты производительности выделены меткой performance и запускаются отдельно:

```powershell
ctest -L performance -V
```

- Для запуска юнит-тестов:

```powershell
ctest -L unit -V
```

### Пример вывода производительности

```powershell
[InsertMillionEntries] Duration: 2200 ms
[ReadRandomEntries] Duration: 6 ms
```

## Примечания

- Используется комбинация std::map и unordered_map для ускорения доступа и поддержки сортировки.

- TTL обрабатывается через std::chrono::time_point.