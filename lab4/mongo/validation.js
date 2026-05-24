// ============================================================
// Валидация схем MongoDB — Система управления рецептами
// Используется $jsonSchema для обеих коллекций
// ============================================================

// ------------------------------------------------------------
// Валидация коллекции recipes
// ------------------------------------------------------------
db.runCommand({
  collMod: "recipes",
  validator: {
    $jsonSchema: {
      bsonType: "object",
      title: "Recipe Validation",
      required: ["title", "instructions", "cooking_time_minutes", "author_id", "ingredients"],
      properties: {
        _id: {
          bsonType: "objectId",
          description: "Уникальный идентификатор рецепта"
        },
        title: {
          bsonType: "string",
          minLength: 1,
          maxLength: 200,
          description: "Название рецепта — обязательно, от 1 до 200 символов"
        },
        description: {
          bsonType: "string",
          maxLength: 1000,
          description: "Описание рецепта — не более 1000 символов"
        },
        instructions: {
          bsonType: "string",
          minLength: 1,
          maxLength: 10000,
          description: "Инструкция приготовления — обязательно, от 1 до 10000 символов"
        },
        cooking_time_minutes: {
          bsonType: "int",
          minimum: 1,
          maximum: 10080,
          description: "Время приготовления в минутах — от 1 до 10080 (1 неделя)"
        },
        author_id: {
          bsonType: "objectId",
          description: "ID автора рецепта — обязателен"
        },
        favorite_count: {
          bsonType: "int",
          minimum: 0,
          description: "Количество добавлений в избранное"
        },
        created_at: {
          bsonType: "date",
          description: "Дата создания рецепта"
        },
        ingredients: {
          bsonType: "array",
          minItems: 0,
          description: "Список ингредиентов рецепта",
          items: {
            bsonType: "object",
            required: ["name", "amount"],
            properties: {
              name: {
                bsonType: "string",
                minLength: 1,
                maxLength: 200,
                description: "Название ингредиента — обязательно, от 1 до 200 символов"
              },
              amount: {
                bsonType: "string",
                minLength: 1,
                maxLength: 100,
                description: "Количество — обязательно, от 1 до 100 символов"
              },
              unit: {
                bsonType: "string",
                maxLength: 50,
                description: "Единица измерения — не более 50 символов"
              }
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
// Валидация коллекции users
// ------------------------------------------------------------
db.runCommand({
  collMod: "users",
  validator: {
    $jsonSchema: {
      bsonType: "object",
      title: "User Validation",
      required: ["username", "password_hash", "email", "first_name", "last_name"],
      properties: {
        _id: {
          bsonType: "objectId",
          description: "Уникальный идентификатор пользователя"
        },
        username: {
          bsonType: "string",
          minLength: 3,
          maxLength: 50,
          description: "Логин пользователя — от 3 до 50 символов, уникальный"
        },
        password_hash: {
          bsonType: "string",
          description: "Хэш пароля"
        },
        email: {
          bsonType: "string",
          pattern: "^[\\w\\.-]+@[\\w\\.-]+\\.\\w+$",
          description: "Email пользователя — должен быть валидным email"
        },
        first_name: {
          bsonType: "string",
          minLength: 1,
          maxLength: 100,
          description: "Имя пользователя — от 1 до 100 символов"
        },
        last_name: {
          bsonType: "string",
          minLength: 1,
          maxLength: 100,
          description: "Фамилия пользователя — от 1 до 100 символов"
        },
        recipe_ids: {
          bsonType: "array",
          description: "Массив ID рецептов, созданных пользователем",
          items: {
            bsonType: "objectId"
          }
        },
        favorite_recipe_ids: {
          bsonType: "array",
          description: "Массив ID рецептов в избранном",
          items: {
            bsonType: "objectId"
          }
        }
      },
      additionalProperties: false
    }
  },
  validationLevel: "strict",
  validationAction: "error"
});
