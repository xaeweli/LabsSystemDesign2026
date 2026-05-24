// ============================================================
// Инициализация MongoDB — Система управления рецептами
// Создание коллекций, индексов, валидации и тестовых данных
// ============================================================

// Переключение на БД recipe_manager
db = db.getSiblingDB("recipe_manager");

// ------------------------------------------------------------
// 1. Создание коллекции recipes с валидацией
// ------------------------------------------------------------
db.createCollection("recipes", {
  validator: {
    $jsonSchema: {
      bsonType: "object",
      required: ["title", "instructions", "cooking_time_minutes", "author_id", "ingredients"],
      properties: {
        _id: { bsonType: "objectId" },
        title: {
          bsonType: "string",
          minLength: 1,
          maxLength: 200,
          description: "Recipe title — required, 1-200 chars"
        },
        description: {
          bsonType: "string",
          maxLength: 1000,
          description: "Recipe description — max 1000 chars"
        },
        instructions: {
          bsonType: "string",
          minLength: 1,
          maxLength: 10000,
          description: "Cooking instructions — required, 1-10000 chars"
        },
        cooking_time_minutes: {
          bsonType: "int",
          minimum: 1,
          maximum: 10080,
          description: "Cooking time — 1-10080 minutes"
        },
        author_id: { bsonType: "objectId" },
        favorite_count: {
          bsonType: "int",
          minimum: 0,
          description: "Favorite count"
        },
        created_at: { bsonType: "date" },
        ingredients: {
          bsonType: "array",
          items: {
            bsonType: "object",
            required: ["name", "amount"],
            properties: {
              name: { bsonType: "string", minLength: 1, maxLength: 200 },
              amount: { bsonType: "string", minLength: 1, maxLength: 100 },
              unit: { bsonType: "string", maxLength: 50 }
            },
            additionalProperties: false
          }
        }
      },
      additionalProperties: false
    }
  },
  validationLevel: "strict",
  validationAction: "error"
});

// ------------------------------------------------------------
// 2. Создание коллекции users с валидацией
// ------------------------------------------------------------
db.createCollection("users", {
  validator: {
    $jsonSchema: {
      bsonType: "object",
      required: ["username", "password_hash", "email", "first_name", "last_name"],
      properties: {
        _id: { bsonType: "objectId" },
        username: {
          bsonType: "string",
          minLength: 3,
          maxLength: 50,
          description: "Username — 3-50 chars, unique"
        },
        password_hash: { bsonType: "string" },
        email: {
          bsonType: "string",
          pattern: "^[\\w\\.-]+@[\\w\\.-]+\\.\\w+$",
          description: "Valid email required"
        },
        first_name: { bsonType: "string", minLength: 1, maxLength: 100 },
        last_name: { bsonType: "string", minLength: 1, maxLength: 100 },
        recipe_ids: {
          bsonType: "array",
          items: { bsonType: "objectId" }
        },
        favorite_recipe_ids: {
          bsonType: "array",
          items: { bsonType: "objectId" }
        }
      },
      additionalProperties: false
    }
  },
  validationLevel: "strict",
  validationAction: "error"
});

// ------------------------------------------------------------
// 3. Создание уникальных индексов
// ------------------------------------------------------------
db.users.createIndex({ username: 1 }, { unique: true });
db.users.createIndex({ email: 1 }, { unique: true });

// ------------------------------------------------------------
// 4. Создание индексов для поиска
// ------------------------------------------------------------
db.recipes.createIndex({ title: 1 });
db.recipes.createIndex({ author_id: 1 });
db.recipes.createIndex({ created_at: -1 });
db.recipes.createIndex({ "ingredients.name": 1 });
db.users.createIndex({ first_name: 1, last_name: 1 });
