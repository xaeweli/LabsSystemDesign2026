-- 1. Создание нового пользователя
--    Auth DB: регистрация учётных данных
--    Users DB: создание профиля

-- Шаг 1.1: Auth DB — создание учётной записи
INSERT INTO user_credentials (id, login, password_hash, email, facebook_id, google_id)
VALUES (
    gen_random_uuid(),
    'new_user',
    'hash_123456',
    'new@example.com',
    NULL,
    NULL
)
RETURNING id, login, email;

-- Шаг 1.2: Users DB — создание профиля (с тем же id)
INSERT INTO user_profiles (id, username, first_name, last_name)
VALUES (
    'id_из_предыдущего_шага',
    'new_user',
    'Новый',
    'Пользователь'
)
RETURNING id, username, first_name, last_name;



-- 2. Поиск пользователя по логину
--    Auth DB: найти id по login
--    Users DB: получить профиль по id

-- Шаг 2.1: Auth DB
SELECT id, login, email
FROM user_credentials
WHERE login = 'chef_ivan';

-- Шаг 2.2: Users DB
SELECT id, username, first_name, last_name
FROM user_profiles
WHERE id = 'a0000000-0000-0000-0000-000000000001';



-- 3. Поиск пользователя по маске имени и фамилии
--    Users DB: поиск по профилям


SELECT id, username, first_name, last_name
FROM user_profiles
WHERE first_name ILIKE 'Ив%'
  AND last_name  ILIKE 'Пет%'
ORDER BY username;


-- 4. Создание рецепта
--    Recipes DB: вставка рецепта
--    Users DB: добавление в список рецептов пользователя


-- Шаг 4.1: Recipes DB
INSERT INTO recipes (id, title, description, instructions, cooking_time_minutes, author_id)
VALUES (
    gen_random_uuid(),
    'Новый рецепт',
    'Описание нового рецепта',
    'Инструкция по приготовлению.',
    45,
    'a0000000-0000-0000-0000-000000000001'
)
RETURNING id, title, description, instructions, cooking_time_minutes, author_id, created_at;

-- Шаг 4.2: Users DB
INSERT INTO user_recipes (user_id, recipe_id)
VALUES (
    'a0000000-0000-0000-0000-000000000001',
    'id_рецепта_из_предыдущего_шага'
);



-- 5. Получение списка рецептов
--    Recipes DB: все рецепты с ингредиентами


SELECT id, title, description, instructions, cooking_time_minutes,
       author_id, favorite_count, created_at
FROM recipes
ORDER BY title;



-- 6. Поиск рецептов по названию
--    Recipes DB: ILIKE по названию


SELECT id, title, description, instructions, cooking_time_minutes,
       author_id, favorite_count, created_at
FROM recipes
WHERE title ILIKE '%борщ%'
ORDER BY title;



-- 7. Добавление ингредиента в рецепт
--    Recipes DB: вставка ингредиента


INSERT INTO ingredients (id, recipe_id, name, amount, unit)
VALUES (
    gen_random_uuid(),
    'b0000000-0000-0000-0000-000000000001',
    'Сметана',
    '200',
    'мл'
)
RETURNING id, name, amount, unit;



-- 8. Получение ингредиентов рецепта
--    Recipes DB: SELECT по recipe_id


SELECT i.id, i.name, i.amount, i.unit
FROM ingredients i
WHERE i.recipe_id = 'b0000000-0000-0000-0000-000000000001'
ORDER BY i.name;



-- 9. Получение рецептов пользователя
--    Recipes DB: SELECT по author_id


SELECT r.id, r.title, r.description, r.instructions,
       r.cooking_time_minutes, r.author_id, r.favorite_count, r.created_at
FROM recipes r
WHERE r.author_id = 'a0000000-0000-0000-0000-000000000001'
ORDER BY r.title;



-- 10. Добавление рецепта в избранное
--     Users DB: INSERT в user_favorites
--     Recipes DB: увеличение favorite_count


-- Шаг 10.1: Users DB
INSERT INTO user_favorites (user_id, recipe_id)
VALUES ('a0000000-0000-0000-0000-000000000001', 'b0000000-0000-0000-0000-000000000009')
ON CONFLICT DO NOTHING;

-- Шаг 10.2: Recipes DB
UPDATE recipes
SET favorite_count = favorite_count + 1
WHERE id = 'b0000000-0000-0000-0000-000000000009';



-- 11. Получение избранных рецептов пользователя
--      Users DB: список recipe_id из user_favorites
--      Recipes DB: получение рецептов по списку id


-- Шаг 11.1: Users DB
SELECT recipe_id FROM user_favorites
WHERE user_id = 'a0000000-0000-0000-0000-000000000001';

-- Шаг 11.2: Recipes DB (IN по списку из шага 11.1)
SELECT id, title, description, instructions, cooking_time_minutes,
       author_id, favorite_count, created_at
FROM recipes
WHERE id IN ('b0000000-0000-0000-0000-000000000003', 'b0000000-0000-0000-0000-000000000005')
ORDER BY title;



-- 12. Обновление рецепта
--     Recipes DB: UPDATE


UPDATE recipes
SET title = 'Борщ с пампушками',
    description = 'Традиционный борщ с чесночными пампушками'
WHERE id = 'b0000000-0000-0000-0000-000000000001'
RETURNING id, title, description;



-- 13. Запрос с JOIN (в рамках одной БД):
--     Recipes DB: рецепты, отсортированные по времени готовки


SELECT r.title, r.cooking_time_minutes, r.author_id, r.favorite_count
FROM recipes r
ORDER BY r.cooking_time_minutes;



-- 14. Агрегация: количество рецептов у каждого пользователя
--     Users DB + Recipes DB (cross-database, объединение в коде)


-- Шаг 14.1: Users DB — все пользователи
SELECT id, username FROM user_profiles ORDER BY username;

-- Шаг 14.2: Recipes DB — количество рецептов на автора
SELECT author_id, COUNT(id) AS recipe_count
FROM recipes
GROUP BY author_id
ORDER BY recipe_count DESC;



-- 15. Агрегация: топ избранных рецептов
--     Recipes DB: favorite_count уже денормализован


SELECT id, title, favorite_count
FROM recipes
ORDER BY favorite_count DESC
LIMIT 5;
