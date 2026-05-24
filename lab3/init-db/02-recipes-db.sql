-- Recipes Database: рецепты и ингредиенты
\c recipes_db

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

CREATE INDEX IF NOT EXISTS idx_recipes_author_id ON recipes(author_id);
CREATE INDEX IF NOT EXISTS idx_recipes_title     ON recipes(title);
CREATE INDEX IF NOT EXISTS idx_recipes_created_at ON recipes(created_at DESC);
CREATE INDEX IF NOT EXISTS idx_ingredients_recipe_id ON ingredients(recipe_id);
CREATE INDEX IF NOT EXISTS idx_ingredients_name      ON ingredients(name);
