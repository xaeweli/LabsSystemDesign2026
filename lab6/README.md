# Лабораторная работа 6: Проектирование Event-Driven архитектуры

**Вариант 23** — Система управления рецептами (Allrecipes)

## Тема работы

Проектирование и реализация Event-Driven архитектуры, RabbitMQ и CQRS.

## Стек

- C++20, userver
- RabbitMQ (event broker)
- In-memory storage (CQRS: write model + read model)

## Структура проекта

```
lab6/
├── README.md
├── CMakeLists.txt
├── Dockerfile
├── docker-compose.yaml
├── event_driven_design.md     # Event-Driven архитектура
├── event_catalog.md           # Каталог событий
├── .dockerignore
├── configs/
│   ├── static_config.yaml
│   └── config_vars.yaml
├── src/
│   ├── main.cpp
│   ├── api_types.hpp
│   ├── api_utils.hpp/.cpp
│   ├── event_types.hpp        # Типы событий
│   ├── event_producer.hpp/.cpp # Producer (RabbitMQ)
│   ├── event_consumer.hpp/.cpp # Consumer (RabbitMQ)
│   ├── recipe_storage.hpp/.cpp # Write model (CQRS command side)
│   ├── recipe_read_model.hpp/.cpp  # Read model (CQRS query side)
│   └── handlers.hpp/.cpp      # HTTP-обработчики с событиями
```

## События

| Событие | Producer | Consumers |
|---------|----------|-----------|
| `UserCreated` | `POST /users` | Read Model Updater |
| `RecipeCreated` | `POST /recipes` | Read Model, Search Index |
| `IngredientAdded` | `POST /recipes/{id}/ingredients` | Read Model, Search Index |
| `RecipeFavorited` | `POST /users/{id}/favorites` | Read Model |

## CQRS

- **Write Model**: `RecipeStorage` — команды (создание, обновление)
- **Read Model**: `RecipeReadModel` — запросы (списки, поиск)
- **Синхронизация**: через события RabbitMQ

## API

| Метод | Путь | Описание |
|-------|------|----------|
| POST | `/users` | Создать пользователя (публикует `UserCreated`) |
| GET | `/users/login/{username}` | Поиск по логину |
| GET | `/users/search` | Поиск по маске имени/фамилии |
| POST | `/recipes` | Создать рецепт (публикует `RecipeCreated`) |
| GET | `/recipes` | Список рецептов (из read model) |
| GET | `/recipes/search` | Поиск рецептов (из read model) |
| POST | `/recipes/{id}/ingredients` | Добавить ингредиент (публикует `IngredientAdded`) |
| GET | `/recipes/{id}/ingredients` | Ингредиенты рецепта |
| GET | `/users/{id}/recipes` | Рецепты пользователя |
| POST | `/users/{id}/favorites` | Добавить в избранное (публикует `RecipeFavorited`) |
| GET | `/users/{id}/favorites` | Избранные рецепты |

## Запуск

```bash
docker-compose up --build
```

Сервис: `http://localhost:8080`
RabbitMQ Management: `http://localhost:15672` (guest/guest)

## Тестирование интеграции

### 1. Запуск системы
```bash
docker-compose up --build
```

### 2. Проверка RabbitMQ
```
Откройте http://localhost:15672 (guest/guest)
Проверьте exchange "recipe.events" и очередь "recipe.read-model"
```

### 3. Создание пользователя (проверка UserCreated event)
```bash
# Создать пользователя (публикуется событие UserCreated)
curl -s -X POST localhost:8080/users \
  -H 'Content-Type: application/json' \
  -d '{"username":"chef23","first_name":"Ivan","last_name":"Petrov","email":"chef@test.com","password":"secret123"}'

# Ожидаемый ответ: 201 Created с JSON пользователя
# В логах API: "Published event UserCreated"
```

### 4. Создание рецепта (проверка RecipeCreated event)
```bash
# Создать рецепт (публикуется событие RecipeCreated)
curl -s -X POST localhost:8080/recipes \
  -H 'Content-Type: application/json' \
  -d '{"user_id":"<user_id>","title":"Apple Pie","description":"Classic dessert","instructions":"Bake for 60 min","cooking_time_minutes":60}'

# Ожидаемый ответ: 201 Created с JSON рецепта
# В логах API: "Published event RecipeCreated"
```

### 5. Добавление ингредиента (проверка IngredientAdded event)
```bash
curl -s -X POST localhost:8080/recipes/<recipe_id>/ingredients \
  -H 'Content-Type: application/json' \
  -d '{"name":"Apple","amount":"4","unit":"pcs"}'

# Ожидаемый ответ: 201 Created с JSON ингредиента
# В логах API: "Published event IngredientAdded"
```

### 6. Чтение из Read Model (CQRS)
```bash
# Получить список рецептов (из read model, обновлён через события)
curl -s localhost:8080/recipes

# Ожидаемый ответ: JSON массив RecipeSummary с ingredient_count
# Read model синхронизируется через события RabbitMQ
```

### 7. Проверка в RabbitMQ Management UI
1. Откройте http://localhost:15672 (guest/guest)
2. Перейдите в "Exchanges" → "recipe.events" → "Bindings"
3. Должны быть привязаны очереди: `recipe.read-model`
4. Перейдите в "Queues" → "recipe.read-model" → "Get messages"
5. Должны отображаться опубликованные события

### Ожидаемые логи при успешном тесте
```
[info] Connected to RabbitMQ at rabbitmq:5672, exchange: recipe.events
[info] EventConsumer connected, consuming from queue: recipe.read-model
[info] Published event UserCreated (routing_key=user.created)
[info] Consumer: handling UserCreated event
[info] Published event RecipeCreated (routing_key=recipe.created)
[info] Consumer: handling RecipeCreated event
[info] Published event IngredientAdded (routing_key=recipe.ingredient.added)
[info] Consumer: handling IngredientAdded event
```

## Примеры

```bash
# Создать пользователя
curl -X POST localhost:8080/users \
  -H 'Content-Type: application/json' \
  -d '{"username":"chef23","first_name":"Ivan","last_name":"Petrov","email":"chef@test.com","password":"secret123"}'

# Создать рецепт
curl -X POST localhost:8080/recipes \
  -H 'Content-Type: application/json' \
  -d '{"user_id":"<user_id>","title":"Apple Pie","description":"Classic","instructions":"Bake","cooking_time_minutes":60}'

# Получить рецепты (из read model)
curl localhost:8080/recipes
```
