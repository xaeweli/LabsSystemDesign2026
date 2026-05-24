# Лабораторная работа 5: Оптимизация производительности и Rate Limiting

**Вариант 23** — Система управления рецептами (Allrecipes)

## Тема работы

Проектирование и реализация системы кэширования и rate limiting
для повышения производительности и защиты от перегрузок.

## Стек

- C++20, userver
- Redis (кэширование)
- In-memory rate limiter (Sliding Window)

## Структура проекта

```
lab5/
├── README.md
├── CMakeLists.txt
├── Dockerfile
├── docker-compose.yaml
├── configs/
│   ├── static_config.yaml              # userver + Redis компоненты
│   └── config_vars.yaml
├── src/
│   ├── main.cpp
│   ├── api_types.hpp
│   ├── api_utils.hpp/.cpp
│   ├── recipe_storage.hpp/.cpp         # In-memory хранилище
│   ├── rate_limiter.hpp/.cpp           # Sliding Window rate limiter
│   ├── cache_manager.hpp/.cpp          # Redis Cache-Aside
│   └── handlers.hpp/.cpp               # HTTP handlers с кэшем и rate limit
└── performance_design.md               # Дизайн-документ
```

## Кэширование (Redis)

Стратегия: **Cache-Aside** (Lazy Loading)

| Endpoint | Ключ кэша | TTL |
|----------|-----------|-----|
| `GET /recipes` | `recipes:list` | 5 мин |
| `GET /recipes/search?title=X` | `recipes:search:X` | 5 мин |
| `GET /users/login/{name}` | `user:login:{name}` | 10 мин |

## Rate Limiting

Алгоритм: **Sliding Window**

| Endpoint | Лимит | Окно |
|----------|-------|------|
| `GET /recipes` | 100 | 60 сек |
| `GET /recipes/search` | 100 | 60 сек |
| `POST /recipes` | 50 | 60 сек |
| `POST /users` | 20 | 60 сек |
| `GET /users/login/{username}` | 200 | 60 сек |
| `GET /users/search` | 200 | 60 сек |
| `POST /recipes` | 50 | 60 сек |
| `POST /recipes/{id}/ingredients` | 60 | 60 сек |
| `GET /recipes/{id}/ingredients` | 200 | 60 сек |
| `GET /users/{id}/recipes` | 200 | 60 сек |
| `POST /users/{id}/favorites` | 30 | 60 сек |
| `GET /users/{id}/favorites` | 100 | 60 сек |

Заголовки ответа: `X-RateLimit-Limit`, `X-RateLimit-Remaining`, `X-RateLimit-Reset`

## API

| Метод | Путь | Описание |
|-------|------|----------|
| POST | `/users` | Создать пользователя |
| GET | `/users/login/{username}` | Поиск по логину |
| GET | `/users/search` | Поиск по маске имени/фамилии |
| POST | `/recipes` | Создать рецепт |
| GET | `/recipes` | Список рецептов (cached) |
| GET | `/recipes/search` | Поиск рецептов (cached) |
| POST | `/recipes/{id}/ingredients` | Добавить ингредиент |
| GET | `/recipes/{id}/ingredients` | Ингредиенты рецепта |
| GET | `/users/{id}/recipes` | Рецепты пользователя |
| POST | `/users/{id}/favorites` | Добавить в избранное |
| GET | `/users/{id}/favorites` | Избранные рецепты |

## Запуск

```bash
docker-compose up --build
```

Сервис: `http://localhost:8080`
Redis: `localhost:6379`

## Примеры

```bash
# Создать пользователя
curl -X POST localhost:8080/users \
  -H 'Content-Type: application/json' \
  -d '{"username":"new_user","first_name":"Иван","last_name":"Петров","email":"ivan@test.com","password":"secret123"}'

# Получить список рецептов (с кэшем)
curl -v localhost:8080/recipes  # headers: X-RateLimit-*

# Поиск рецептов
curl -v localhost:8080/recipes/search?title=*борщ*
```

## Проверка rate limiting

```bash
# Быстрый прогон 30 запросов (после 20-го будет 429)
for ($i=0; $i -lt 30; $i++) {
  curl -s -o /dev/null -w "%{http_code} " -X POST localhost:8080/users \
    -H 'Content-Type: application/json' \
    -d '{"username":"user'$i'","first_name":"U","last_name":"$i","email":"u'$i'@test.com","password":"123456"}'
}
```
