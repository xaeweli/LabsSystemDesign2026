# Лабораторная работа 3: Проектирование и реализация базы данных

**Вариант 23** — Система управления рецептами (Allrecipes)


## Архитектура баз данных

Система состоит из трёх логически разделённых баз данных PostgreSQL

| База данных               | Назначение                                                    |
|---------------------------|---------------------------------------------------------------|
| **Recipes Database**      | Рецепты (id, название, ингредиенты, автор, кол-во избранного) |
| **Users Database**        | Профили пользователей, их рецепты, избранное                  |
| **User Auth Database**    | Аутентификация (логин, пароль, email, Facebook, Google)       |

## Схема базы данных

### Recipes Database

#### `recipes`

| Поле                | Тип      | Ограничения                    |
|---------------------|----------|---------------------------------|
| id                  | UUID     | PRIMARY KEY                     |
| title               | VARCHAR  | NOT NULL, CHECK (1..200)        |
| description         | TEXT     | DEFAULT '', CHECK (<= 1000)     |
| instructions        | TEXT     | NOT NULL, CHECK (1..10000)      |
| cooking_time_minutes| INTEGER  | NOT NULL, CHECK (1..10080)      |
| author_id           | UUID     | NOT NULL                        |
| favorite_count      | INTEGER  | DEFAULT 0                       |
| created_at          | TIMESTAMP| DEFAULT CURRENT_TIMESTAMP       |

#### `ingredients`

| Поле      | Тип      | Ограничения                      |
|-----------|----------|----------------------------------|
| id        | UUID     | PRIMARY KEY                      |
| recipe_id | UUID     | NOT NULL, FK → recipes CASCADE   |
| name      | VARCHAR  | NOT NULL, CHECK (1..200)         |
| amount    | VARCHAR  | NOT NULL, CHECK (1..100)         |
| unit      | VARCHAR  | DEFAULT '', CHECK (<= 50)        |

### Users Database

#### `user_profiles`

| Поле       | Тип         | Ограничения                     |
|------------|-------------|----------------------------------|
| id         | UUID        | PRIMARY KEY                      |
| username   | VARCHAR(50) | NOT NULL, UNIQUE, CHECK (3..50)  |
| first_name | VARCHAR(100)| NOT NULL, CHECK (1..100)         |
| last_name  | VARCHAR(100)| NOT NULL, CHECK (1..100)         |

#### `user_recipes`

| Поле      | Тип  | Ограничения                    |
|-----------|------|---------------------------------|
| user_id   | UUID | NOT NULL, PK                    |
| recipe_id | UUID | NOT NULL, PK                    |

#### `user_favorites`

| Поле      | Тип  | Ограничения                    |
|-----------|------|---------------------------------|
| user_id   | UUID | NOT NULL, PK                    |
| recipe_id | UUID | NOT NULL, PK                    |

### Auth Database

#### `user_credentials`

| Поле          | Тип         | Ограничения                        |
|---------------|-------------|-------------------------------------|
| id            | UUID        | PRIMARY KEY                         |
| login         | VARCHAR(50) | NOT NULL, UNIQUE, CHECK (3..50)     |
| password_hash | VARCHAR(255)| NOT NULL, CHECK (>= 6)              |
| email         | VARCHAR(255)| NOT NULL, UNIQUE, CHECK (email)     |
| facebook_id   | VARCHAR(255)| DEFAULT NULL                        |
| google_id     | VARCHAR(255)| DEFAULT NULL                        |


## API

| Метод | Путь                              | Описание                      | БД                        |
|-------|-----------------------------------|--------------------------------|---------------------------|
| POST  | `/users`                          | Создать пользователя           | Auth DB + Users DB        |
| GET   | `/users/login/{username}`         | Поиск по логину                | Auth DB + Users DB        |
| GET   | `/users/search`                   | Поиск по маске имени/фамилии   | Users DB                  |
| POST  | `/recipes`                        | Создать рецепт                 | Recipes DB + Users DB     |
| GET   | `/recipes`                        | Список рецептов                | Recipes DB                |
| GET   | `/recipes/search`                 | Поиск рецептов по названию     | Recipes DB                |
| POST  | `/recipes/{id}/ingredients`       | Добавить ингредиент            | Recipes DB                |
| GET   | `/recipes/{id}/ingredients`       | Ингредиенты рецепта            | Recipes DB                |
| GET   | `/users/{id}/recipes`             | Рецепты пользователя           | Recipes DB                |
| POST  | `/users/{id}/favorites`           | Добавить в избранное           | Users DB + Recipes DB     |
| GET   | `/users/{id}/favorites`           | Избранные рецепты              | Users DB + Recipes DB     |

## Структура проекта

```
lab3/
├── README.md
├── CMakeLists.txt
├── database/                         # SQL — основной результат работы
│   ├── schema.sql                    #   Схема всех 3 БД
│   ├── data.sql                      #   Тестовые данные
│   ├── queries.sql                   #   Запросы по БД
│   └── optimization.md               #   Отчёт EXPLAIN
├── init-db/                          # Скрипты инициализации для Docker с PostgreSQL
│   ├── 01-create-dbs.sh
│   ├── 02-recipes-db.sql
│   ├── 03-users-db.sql
│   ├── 04-auth-db.sql
│   └── 05-07-data-*.sql
├── configs/
│   ├── static_config.yaml
│   └── config_vars.yaml
├── src/
│   ├── api_types.hpp
│   ├── api_utils.hpp / .cpp
│   ├── handlers.hpp / .cpp
│   ├── recipe_storage.hpp / .cpp     # In-memory, архитектура 3 БД
│   └── main.cpp
├── Dockerfile
└── docker-compose.yaml
```

## Запуск

```bash
docker-compose up --build
```

Сервис будет доступен по адресу `http://localhost:8080`.

Хранилище — in-memory (структурировано по архитектуре 3 БД).
SQL-схемы в `database/` предназначены для развёртывания на PostgreSQL.

## Примеры запросов

```bash
# Создать пользователя
curl -X POST localhost:8080/users \
  -H 'Content-Type: application/json' \
  -d '{"username":"new_user","first_name":"Иван","last_name":"Петров","email":"ivan@test.com","password":"secret123"}'

# Создать рецепт
curl -X POST localhost:8080/recipes \
  -H 'Content-Type: application/json' \
  -d '{"user_id":"<user_id>","title":"Борщ","instructions":"Сварить...","cooking_time_minutes":90}'

# Добавить ингредиент
curl -X POST localhost:8080/recipes/<recipe_id>/ingredients \
  -H 'Content-Type: application/json' \
  -d '{"name":"Свёкла","amount":"2","unit":"шт"}'
```
