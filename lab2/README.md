# Recipe Manager API на userver

Вариант 23: система управления рецептами по мотивам Allrecipes.

## Стек

C++20

userver, вендорится в репозитории как git submodule

In-memory storage

## Запуск

```bash
cmake -S . -B build
cmake --build build -j
./build/recipe-manager-userver --config configs/static_config.yaml --config_vars configs/config_vars.yaml
```

Альтернатива:

```bash
docker-compose up --build
```

Сервис слушает `http://localhost:8080`.

## API

- `POST /users` - создать пользователя.
- `GET /users/login/{username}` - найти пользователя по логину.
- `GET /users/search?first_name=Iv*&last_name=Pet*` - поиск пользователей по маске имени и фамилии. Поддерживается `*`.
- `POST /recipes` - создать рецепт.
- `GET /recipes` - получить список рецептов.
- `GET /recipes/search?title=*cake*` - поиск рецептов по названию. Поддерживается `*`.
- `POST /recipes/{recipe_id}/ingredients` - добавить ингредиент в рецепт.
- `GET /recipes/{recipe_id}/ingredients` - получить ингредиенты рецепта.
- `GET /users/{user_id}/recipes` - получить рецепты пользователя.
- `POST /users/{user_id}/favorites` - добавить рецепт в избранное.
- `GET /users/{user_id}/favorites` - получить избранные рецепты пользователя.

## Примеры

```bash
curl -X POST localhost:8080/users \
  -H 'Content-Type: application/json' \
  -d '{"username":"chef23","first_name":"Ivan","last_name":"Petrov","email":"chef23@example.com","password":"secret1"}'
```

```bash
curl -X POST localhost:8080/recipes \
  -H 'Content-Type: application/json' \
  -d '{"user_id":"<user_id>","title":"Apple Pie","description":"Classic dessert","instructions":"Bake apples in pastry.","cooking_time_minutes":60}'
```

```bash
curl -X POST localhost:8080/recipes/<recipe_id>/ingredients \
  -H 'Content-Type: application/json' \
  -d '{"name":"Apple","amount":"4","unit":"pcs"}'
```
