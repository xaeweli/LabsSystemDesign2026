# Лабораторная работа 4: Проектирование и работа с MongoDB

**Вариант 23** — Система управления рецептами (Allrecipes)

## Тема работы

Проектирование документной модели в MongoDB, проектирование вложенных документов,
реализация CRUD операций и агрегации данных. Интеграция MongoDB с API на userver.

## Структура проекта

```
lab4/
├── README.md
├── CMakeLists.txt                          # Сборка userver с MongoDB
├── Dockerfile                              # Docker-образ API
├── docker-compose.yaml                     # API + MongoDB
├── .dockerignore
├── configs/
│   ├── static_config.yaml                  # Конфигурация компонентов userver
│   └── config_vars.yaml                    # Переменные конфигурации
├── src/
│   ├── main.cpp                            # Точка входа
│   ├── api_types.hpp                       # Типы данных (User, Recipe, Ingredient)
│   ├── api_utils.hpp / api_utils.cpp       # Вспомогательные функции
│   ├── handlers.hpp / handlers.cpp         # HTTP-обработчики
│   └── mongo_storage.hpp / mongo_storage.cpp # MongoDB-хранилище
├── mongo/
│   ├── schema_design.md                    # Документная модель (embedded vs references)
│   ├── data.json                           # Тестовые данные (10+ записей)
│   ├── queries.js                          # MongoDB запросы (CRUD + агрегация)
│   ├── validation.js                       # Валидация схем $jsonSchema
│   └── init-mongo.js                       # Инициализация MongoDB
└── init-db/
```

## Документная модель

| Коллекция    | Документы                                                     |
|-------------|---------------------------------------------------------------|
| **recipes** | Вложенные `ingredients`, ссылки на `author_id`                |
| **users**   | Профиль + учётные данные, массивы `recipe_ids`, `favorite_recipe_ids` |

### Выбор Embedded vs References

- **Recipe → Ingredient**: **Embedded** — ингредиенты всегда с рецептом, bounded set
- **User → Recipe**: **References** — рецепты самостоятельные, many-to-many
- **User → Favorites**: **References** — many-to-many, большой массив

## API

| Метод | Путь                              | Описание                      |
|-------|-----------------------------------|--------------------------------|
| POST  | `/users`                          | Создать пользователя           |
| GET   | `/users/login/{username}`         | Поиск по логину                |
| GET   | `/users/search`                   | Поиск по маске имени/фамилии   |
| POST  | `/recipes`                        | Создать рецепт                 |
| GET   | `/recipes`                        | Список рецептов                |
| GET   | `/recipes/search`                 | Поиск рецептов по названию     |
| POST  | `/recipes/{id}/ingredients`       | Добавить ингредиент            |
| GET   | `/recipes/{id}/ingredients`       | Ингредиенты рецепта            |
| GET   | `/users/{id}/recipes`             | Рецепты пользователя           |
| POST  | `/users/{id}/favorites`           | Добавить в избранное           |
| GET   | `/users/{id}/favorites`           | Избранные рецепты              |

## Запуск

```bash
docker-compose up --build
```

Сервис будет доступен по адресу `http://localhost:8080`.
MongoDB доступна на `localhost:27017`.

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

## MongoDB

Инициализация MongoDB (`init-mongo.js`):
- Создание коллекций `recipes` и `users`
- Валидация схем через `$jsonSchema`
- Индексы для поиска

Тестовые данные (`data.json`): 10 пользователей и 12 рецептов.
