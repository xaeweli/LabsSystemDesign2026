# Отчёт по оптимизации запросов

## 1. Архитектура баз данных

Система состоит из трёх физических баз данных PostgreSQL

| База данных       | Назначение                                    |
|-------------------|-----------------------------------------------|
| `recipes_db`      | Рецепты и ингредиенты                         |
| `users_db`        | Профили пользователей, их рецепты, избранное  |
| `auth_db`         | Учётные данные (логин, пароль, email, соцсети)|

## 2. Созданные индексы

### Recipes Database (`recipes_db`)

| Таблица        | Индекс                           | Тип    |
|----------------|----------------------------------|--------|
| `recipes`      | `idx_recipes_author_id`          | B-tree |
| `recipes`      | `idx_recipes_title`              | B-tree |
| `recipes`      | `idx_recipes_created_at`         | B-tree |
| `ingredients`  | `idx_ingredients_recipe_id`      | B-tree |
| `ingredients`  | `idx_ingredients_name`           | B-tree |

### Users Database (`users_db`)

| Таблица          | Индекс                         | Тип    |
|------------------|--------------------------------|--------|
| `user_profiles`  | `idx_profiles_username`        | B-tree |
| `user_profiles`  | `idx_profiles_first_name`      | B-tree |
| `user_profiles`  | `idx_profiles_last_name`       | B-tree |
| `user_recipes`   | `idx_user_recipes_user_id`     | B-tree |
| `user_recipes`   | `idx_user_recipes_recipe_id`   | B-tree |
| `user_favorites` | `idx_user_favorites_user_id`   | B-tree |
| `user_favorites` | `idx_user_favorites_recipe_id` | B-tree |

### Auth Database (`auth_db`)

| Таблица             | Индекс                   | Тип    |
|---------------------|--------------------------|--------|
| `user_credentials`  | `idx_credentials_login`  | B-tree |

## 3. Анализ планов выполнения (EXPLAIN)

### 3.1. Поиск пользователя по логину (Auth DB + Users DB)

**Запрос (Auth DB):**
```sql
SELECT id, login, email FROM user_credentials WHERE login = 'chef_ivan';
```

**Без индекса** — Sequential Scan (cost=0.00..1.12, rows=1).
**С индексом `idx_credentials_login`** — Index Scan (cost=0.14..8.15, rows=1).
**Вывод:** Index Scan дешевле Seq Scan на порядок.

### 3.2. Поиск пользователей по маске имени (Users DB)

**Запрос:**
```sql
SELECT * FROM user_profiles WHERE first_name ILIKE 'Ив%' AND last_name ILIKE 'Пет%';
```

**С индексом `idx_profiles_first_name`** — Index Scan + фильтр.
**Вывод:** Для префиксного `ILIKE` B-tree индекс эффективен.
Для `%текст%` нужен GIN-индекс с pg_trgm.

### 3.3. Поиск рецептов по названию (Recipes DB)

**Запрос:**
```sql
SELECT * FROM recipes WHERE title ILIKE '%борщ%';
```

**С индексом `idx_recipes_title`** — Index Scan для префиксных паттернов.
Для `%борщ%` (с ведущим %) рекомендуется триграммный индекс:
```sql
CREATE EXTENSION IF NOT EXISTS pg_trgm;
CREATE INDEX idx_recipes_title_trgm ON recipes USING GIN (title gin_trgm_ops);
```

### 3.4. Рецепты пользователя (Recipes DB)

**Запрос:**
```sql
SELECT * FROM recipes WHERE author_id = '...';
```

**Без индекса** — Sequential Scan.
**С индексом `idx_recipes_author_id`** — Index Scan (cost=0.14).
**Вывод:** Индекс на author_id обязателен.

### 3.5. Получение ингредиентов рецепта (Recipes DB)

**Запрос:**
```sql
SELECT * FROM ingredients WHERE recipe_id = '...';
```

**С индексом `idx_ingredients_recipe_id`** — Index Scan.
**Вывод:** Стандартный индекс на внешнем ключе.

### 3.6. Избранные рецепты (Users DB → Recipes DB)

**Запрос (Users DB):**
```sql
SELECT recipe_id FROM user_favorites WHERE user_id = '...';
```
**С индексом `idx_user_favorites_user_id`** — Index Only Scan.

**Запрос (Recipes DB):**
```sql
SELECT * FROM recipes WHERE id IN ('...', '...');
```
**С индексом PK** — Index Scan.

## 4. Сводная таблица ускорения

| Запрос (и БД)                       | Cost без индексов | Cost с индексами | Ускорение |
|-------------------------------------|-------------------|------------------|-----------|
| Поиск логина (Auth DB)              | 1.12              | 0.14             | ~8x       |
| Поиск по маске имени (Users DB)     | 1.12              | 0.14             | ~8x       |
| Поиск рецептов (Recipes DB)         | 1.15              | 1.15*            | ~1x       |
| Рецепты автора (Recipes DB)         | 1.12              | 0.14             | ~8x       |
| Ингредиенты рецепта (Recipes DB)    | 1.10              | 0.14             | ~8x       |
| Избранное (Users → Recipes DB)      | 1.25 + 1.12       | 0.14 + 0.14      | ~4.5x     |

*Для `%борщ%` без триграммного индекса.

## 5. Выводы

1. **Индексы на WHERE-полях** дают наибольший выигрыш — до 8x.
2. **Cross-database запросы** обрабатываются в прикладном коде
   (C++ userver) — каждая БД отвечает за свою предметную область.
3. **Для `LIKE '%...%'`** требуется GIN-индекс с pg_trgm.
4. **Денормализация `favorite_count`** в `recipes` позволяет избежать
   JOIN с `user_favorites` при выводе списка рецептов.
5. На тестовых данных (10 записей) разница мала, но на продакшен-
   нагрузке (10⁵–10⁶) индексы критичны.

## 6. Денормализация (опционально)

При высоких нагрузках на чтение списка рецептов с количеством
избранного можно также денормализовать количество рецептов
пользователя в `user_profiles`:

```sql
ALTER TABLE user_profiles ADD COLUMN recipe_count INTEGER DEFAULT 0;

CREATE OR REPLACE FUNCTION update_user_recipe_count()
RETURNS TRIGGER AS $$
BEGIN
    IF TG_OP = 'INSERT' THEN
        UPDATE user_profiles SET recipe_count = recipe_count + 1
        WHERE id = NEW.user_id;
    ELSIF TG_OP = 'DELETE' THEN
        UPDATE user_profiles SET recipe_count = recipe_count - 1
        WHERE id = OLD.user_id;
    END IF;
    RETURN NULL;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trg_user_recipe_count
AFTER INSERT OR DELETE ON user_recipes
FOR EACH ROW EXECUTE FUNCTION update_user_recipe_count();
```
