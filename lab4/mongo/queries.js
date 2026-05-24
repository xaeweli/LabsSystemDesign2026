// ============================================================
// MongoDB Запросы — Система управления рецептами (Вариант 23)
// Коллекции: recipes, users
// ============================================================

// ------------------------------------------------------------
// 1. Создание нового пользователя
// ------------------------------------------------------------
db.users.insertOne({
  username: "new_user",
  password_hash: "hash_secret123",
  email: "new@example.com",
  first_name: "Новый",
  last_name: "Пользователь",
  recipe_ids: [],
  favorite_recipe_ids: []
});

// ------------------------------------------------------------
// 2. Поиск пользователя по логину
// ------------------------------------------------------------
db.users.find(
  { username: "chef_ivan" },
  { username: 1, first_name: 1, last_name: 1, email: 1, _id: 1 }
);

// ------------------------------------------------------------
// 3. Поиск пользователя по маске имени и фамилии
// ------------------------------------------------------------
db.users.find({
  first_name: { $regex: "^Ив", $options: "i" },
  last_name: { $regex: ".*Пет.*", $options: "i" }
}).sort({ username: 1 });

// ------------------------------------------------------------
// 4. Создание рецепта
// ------------------------------------------------------------
const newRecipeId = ObjectId(); // генерируем новый ID
const authorId = ObjectId("650000000000000000000001");

// Шаг 4.1: Вставка рецепта в коллекцию recipes
db.recipes.insertOne({
  _id: newRecipeId,
  title: "Новый рецепт",
  description: "Описание нового рецепта",
  instructions: "Инструкция по приготовлению.",
  cooking_time_minutes: 45,
  author_id: authorId,
  favorite_count: 0,
  created_at: new Date(),
  ingredients: []
});

// Шаг 4.2: Добавление ID рецепта в массив рецептов пользователя
db.users.updateOne(
  { _id: authorId },
  { $push: { recipe_ids: newRecipeId } }
);

// ------------------------------------------------------------
// 5. Получение списка рецептов (с сортировкой по названию)
// ------------------------------------------------------------
db.recipes.find().sort({ title: 1 });

// ------------------------------------------------------------
// 6. Поиск рецептов по названию (ILIKE)
// ------------------------------------------------------------
db.recipes.find({
  title: { $regex: "борщ", $options: "i" }
}).sort({ title: 1 });

// ------------------------------------------------------------
// 7. Добавление ингредиента в рецепт
// ------------------------------------------------------------
db.recipes.updateOne(
  { _id: ObjectId("660000000000000000000001") },
  {
    $push: {
      ingredients: {
        name: "Сметана",
        amount: "200",
        unit: "мл"
      }
    }
  }
);

// ------------------------------------------------------------
// 8. Получение ингредиентов рецепта
// ------------------------------------------------------------
db.recipes.aggregate([
  { $match: { _id: ObjectId("660000000000000000000001") } },
  { $unwind: "$ingredients" },
  { $sort: { "ingredients.name": 1 } },
  { $project: {
      _id: 0,
      name: "$ingredients.name",
      amount: "$ingredients.amount",
      unit: "$ingredients.unit"
  }}
]);

// Альтернатива (если нужны без агрегации, как вложенный массив):
db.recipes.findOne(
  { _id: ObjectId("660000000000000000000001") },
  { ingredients: 1, _id: 0 }
);

// ------------------------------------------------------------
// 9. Получение рецептов пользователя
// ------------------------------------------------------------
db.recipes.find({
  author_id: ObjectId("650000000000000000000001")
}).sort({ title: 1 });

// ------------------------------------------------------------
// 10. Добавление рецепта в избранное
// ------------------------------------------------------------
const recipeId = ObjectId("660000000000000000000009");
const userId = ObjectId("650000000000000000000001");

// Шаг 10.1: Users DB — добавляем recipe_id в избранное
db.users.updateOne(
  { _id: userId },
  { $addToSet: { favorite_recipe_ids: recipeId } }
);

// Шаг 10.2: Recipes DB — увеличиваем счётчик
db.recipes.updateOne(
  { _id: recipeId },
  { $inc: { favorite_count: 1 } }
);

// ------------------------------------------------------------
// 11. Получение избранных рецептов пользователя
// ------------------------------------------------------------
const user = db.users.findOne(
  { _id: ObjectId("650000000000000000000001") },
  { favorite_recipe_ids: 1, _id: 0 }
);
const favoriteIds = user.favorite_recipe_ids;

db.recipes.find({
  _id: { $in: favoriteIds }
}).sort({ title: 1 });

// ------------------------------------------------------------
// 12. Обновление рецепта
// ------------------------------------------------------------
db.recipes.updateOne(
  { _id: ObjectId("660000000000000000000001") },
  {
    $set: {
      title: "Борщ с пампушками",
      description: "Традиционный борщ с чесночными пампушками"
    }
  }
);

// ------------------------------------------------------------
// 13. Агрегация: топ популярных рецептов (по favorite_count)
// ------------------------------------------------------------
db.recipes.aggregate([
  { $sort: { favorite_count: -1 } },
  { $limit: 5 },
  { $project: {
      _id: 1,
      title: 1,
      favorite_count: 1,
      author_id: 1
  }}
]);

// ------------------------------------------------------------
// 14. Агрегация: количество рецептов у каждого пользователя
// ------------------------------------------------------------
db.recipes.aggregate([
  { $group: { _id: "$author_id", recipe_count: { $sum: 1 } } },
  { $sort: { recipe_count: -1 } },
  { $lookup: {
      from: "users",
      localField: "_id",
      foreignField: "_id",
      as: "user"
  }},
  { $unwind: "$user" },
  { $project: {
      username: "$user.username",
      recipe_count: 1,
      _id: 0
  }}
]);

// ------------------------------------------------------------
// 15. Агрегация: среднее время приготовления по типу блюд
// ------------------------------------------------------------
db.recipes.aggregate([
  { $group: {
      _id: null,
      avg_cooking_time: { $avg: "$cooking_time_minutes" },
      min_time: { $min: "$cooking_time_minutes" },
      max_time: { $max: "$cooking_time_minutes" },
      total_recipes: { $sum: 1 }
  }},
  { $project: {
      _id: 0,
      avg_cooking_time: { $round: ["$avg_cooking_time", 1] },
      min_time: 1,
      max_time: 1,
      total_recipes: 1
  }}
]);

// ------------------------------------------------------------
// 16. Агрегация: самые популярные ингредиенты
// ------------------------------------------------------------
db.recipes.aggregate([
  { $unwind: "$ingredients" },
  { $group: { _id: "$ingredients.name", count: { $sum: 1 } } },
  { $sort: { count: -1 } },
  { $limit: 10 },
  { $project: {
      ingredient: "$_id",
      count: 1,
      _id: 0
  }}
]);

// ------------------------------------------------------------
// 17. Удаление рецепта
// ------------------------------------------------------------
const deleteRecipeId = ObjectId("66000000000000000000000c");
const recipeToDelete = db.recipes.findOne({ _id: deleteRecipeId });
const ownerId = recipeToDelete.author_id;

// Шаг 17.1: Удалить рецепт из коллекции recipes
db.recipes.deleteOne({ _id: deleteRecipeId });

// Шаг 17.2: Удалить id из recipe_ids автора
db.users.updateOne(
  { _id: ownerId },
  { $pull: { recipe_ids: deleteRecipeId } }
);

// Шаг 17.3: Удалить id из избранного всех пользователей
db.users.updateMany(
  { favorite_recipe_ids: deleteRecipeId },
  { $pull: { favorite_recipe_ids: deleteRecipeId } }
);
