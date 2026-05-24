-- Users Database: профили, рецепты пользователя, избранное
\c users_db

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

CREATE TABLE IF NOT EXISTS user_recipes (
    user_id   UUID NOT NULL,
    recipe_id UUID NOT NULL,
    CONSTRAINT pk_user_recipes PRIMARY KEY (user_id, recipe_id)
);

CREATE TABLE IF NOT EXISTS user_favorites (
    user_id   UUID NOT NULL,
    recipe_id UUID NOT NULL,
    CONSTRAINT pk_user_favorites PRIMARY KEY (user_id, recipe_id)
);

CREATE INDEX IF NOT EXISTS idx_profiles_username   ON user_profiles(username);
CREATE INDEX IF NOT EXISTS idx_profiles_first_name ON user_profiles(first_name);
CREATE INDEX IF NOT EXISTS idx_profiles_last_name  ON user_profiles(last_name);
CREATE INDEX IF NOT EXISTS idx_user_recipes_user_id   ON user_recipes(user_id);
CREATE INDEX IF NOT EXISTS idx_user_recipes_recipe_id ON user_recipes(recipe_id);
CREATE INDEX IF NOT EXISTS idx_user_favorites_user_id   ON user_favorites(user_id);
CREATE INDEX IF NOT EXISTS idx_user_favorites_recipe_id ON user_favorites(recipe_id);
