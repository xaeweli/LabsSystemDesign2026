-- База данных 1: Recipes Database
-- Хранит данные о рецептах
-- (id, имя, ингредиенты, автор, кол-во добавленных в избранное)

-- Рецепты
CREATE TABLE IF NOT EXISTS recipes (
    id                   UUID PRIMARY KEY,
    title                VARCHAR(200) NOT NULL,
    description          TEXT         DEFAULT '',
    instructions         TEXT         NOT NULL,
    cooking_time_minutes INTEGER      NOT NULL,
    author_id            UUID         NOT NULL,
    favorite_count       INTEGER      DEFAULT 0,
    created_at           TIMESTAMP    DEFAULT CURRENT_TIMESTAMP,

    CONSTRAINT chk_recipes_title_length CHECK (char_length(title) >= 1 AND char_length(title) <= 200),
    CONSTRAINT chk_recipes_description_length CHECK (char_length(description) <= 1000),
    CONSTRAINT chk_recipes_instructions_length CHECK (char_length(instructions) >= 1 AND char_length(instructions) <= 10000),
    CONSTRAINT chk_recipes_cooking_time CHECK (cooking_time_minutes >= 1 AND cooking_time_minutes <= 10080)
);

-- Ингредиенты
CREATE TABLE IF NOT EXISTS ingredients (
    id        UUID PRIMARY KEY,
    recipe_id UUID         NOT NULL,
    name      VARCHAR(200) NOT NULL,
    amount    VARCHAR(100) NOT NULL,
    unit      VARCHAR(50)  DEFAULT '',

    CONSTRAINT fk_ingredients_recipe FOREIGN KEY (recipe_id) REFERENCES recipes(id) ON DELETE CASCADE,
    CONSTRAINT chk_ingredients_name_length CHECK (char_length(name) >= 1 AND char_length(name) <= 200),
    CONSTRAINT chk_ingredients_amount_length CHECK (char_length(amount) >= 1 AND char_length(amount) <= 100),
    CONSTRAINT chk_ingredients_unit_length CHECK (char_length(unit) <= 50)
);

-- Индексы Recipes DB
CREATE INDEX IF NOT EXISTS idx_recipes_author_id ON recipes(author_id);
CREATE INDEX IF NOT EXISTS idx_recipes_title     ON recipes(title);
CREATE INDEX IF NOT EXISTS idx_recipes_created_at ON recipes(created_at DESC);
CREATE INDEX IF NOT EXISTS idx_ingredients_recipe_id ON ingredients(recipe_id);
CREATE INDEX IF NOT EXISTS idx_ingredients_name      ON ingredients(name);



-- База данных 2: Users Database
-- Хранит данные о пользователях
-- (id, username, name, surname, сохраненные рецепты,
--  добавленные рецепты)


-- Профили пользователей
CREATE TABLE IF NOT EXISTS user_profiles (
    id         UUID PRIMARY KEY,
    username   VARCHAR(50)  NOT NULL,
    first_name VARCHAR(100) NOT NULL,
    last_name  VARCHAR(100) NOT NULL,

    CONSTRAINT uq_profiles_username UNIQUE (username),
    CONSTRAINT chk_profiles_username_length CHECK (char_length(username) >= 3 AND char_length(username) <= 50),
    CONSTRAINT chk_profiles_first_name_length CHECK (char_length(first_name) >= 1 AND char_length(first_name) <= 100),
    CONSTRAINT chk_profiles_last_name_length CHECK (char_length(last_name) >= 1 AND char_length(last_name) <= 100)
);

-- Рецепты, созданные пользователем (список добавленных рецептов)
CREATE TABLE IF NOT EXISTS user_recipes (
    user_id   UUID NOT NULL,
    recipe_id UUID NOT NULL,

    CONSTRAINT pk_user_recipes PRIMARY KEY (user_id, recipe_id)
);

-- Избранные рецепты пользователя (сохраненные рецепты)
CREATE TABLE IF NOT EXISTS user_favorites (
    user_id   UUID NOT NULL,
    recipe_id UUID NOT NULL,

    CONSTRAINT pk_user_favorites PRIMARY KEY (user_id, recipe_id)
);

-- Индексы Users DB
CREATE INDEX IF NOT EXISTS idx_profiles_username   ON user_profiles(username);
CREATE INDEX IF NOT EXISTS idx_profiles_first_name ON user_profiles(first_name);
CREATE INDEX IF NOT EXISTS idx_profiles_last_name  ON user_profiles(last_name);
CREATE INDEX IF NOT EXISTS idx_user_recipes_user_id   ON user_recipes(user_id);
CREATE INDEX IF NOT EXISTS idx_user_recipes_recipe_id ON user_recipes(recipe_id);
CREATE INDEX IF NOT EXISTS idx_user_favorites_user_id   ON user_favorites(user_id);
CREATE INDEX IF NOT EXISTS idx_user_favorites_recipe_id ON user_favorites(recipe_id);



-- База данных 3: User Auth Database
-- Хранит данные пользователя для аутентификации
-- (id, логин, пароль, почта, facebook, google аккаунт)


CREATE TABLE IF NOT EXISTS user_credentials (
    id            UUID PRIMARY KEY,
    login         VARCHAR(50)  NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    email         VARCHAR(255) NOT NULL,
    facebook_id   VARCHAR(255) DEFAULT NULL,
    google_id     VARCHAR(255) DEFAULT NULL,

    CONSTRAINT uq_credentials_login UNIQUE (login),
    CONSTRAINT uq_credentials_email UNIQUE (email),
    CONSTRAINT chk_credentials_login_length CHECK (char_length(login) >= 3 AND char_length(login) <= 50),
    CONSTRAINT chk_credentials_password_length CHECK (char_length(password_hash) >= 6),
    CONSTRAINT chk_credentials_email_format CHECK (email ~* '^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}$')
);

-- Индексы Auth DB
CREATE INDEX IF NOT EXISTS idx_credentials_login ON user_credentials(login);
