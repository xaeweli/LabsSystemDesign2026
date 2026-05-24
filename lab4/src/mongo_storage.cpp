#include "mongo_storage.hpp"

#include <chrono>
#include <regex>

#include <userver/formats/json/value_builder.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/storages/mongo/collection.hpp>
#include <userver/storages/mongo/exception.hpp>
#include <userver/storages/mongo/options.hpp>
#include <userver/utils/mock_now.hpp>
#include <userver/utils/uuid4.hpp>

#include "api_utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace recipe_manager {

namespace {

constexpr std::string_view kRecipesCollection = "recipes";
constexpr std::string_view kUsersCollection = "users";

formats::json::Value MakeBsonDocument(formats::json::ValueBuilder&& builder) {
  return builder.ExtractValue();
}

std::string OidToString(const formats::json::Value& doc) {
  return doc["_id"]["$oid"].As<std::string>("");
}

std::string GetOidJson(const std::string& hex_id) {
  formats::json::ValueBuilder oid;
  oid["$oid"] = hex_id;
  return formats::json::ToString(oid.ExtractValue());
}

}  // namespace

MongoStorage::MongoStorage(const components::ComponentConfig& config, const components::ComponentContext& context)
    : LoggableComponentBase(config, context),
      pool_(context.FindComponent<components::Mongo>("mongo").GetPool()) {}

User MongoStorage::CreateUser(
    const std::string& username,
    const std::string& first_name,
    const std::string& last_name,
    const std::string& email,
    const std::string& password
) {
  ValidateUser(username, first_name, last_name, email, password);

  // Check uniqueness
  auto users = pool_.GetCollection(kUsersCollection);

  auto existing = users.FindOne(formats::json::MakeObject("username", username));
  if (existing) {
    throw ApiException(server::http::HttpStatus::kConflict, "Username already exists");
  }

  existing = users.FindOne(formats::json::MakeObject("email", email));
  if (existing) {
    throw ApiException(server::http::HttpStatus::kConflict, "Email already exists");
  }

  const auto user_id = utils::generators::GenerateUuid();
  const auto now = std::chrono::system_clock::now();

  formats::json::ValueBuilder builder;
  builder["_id"] = user_id;
  builder["username"] = username;
  builder["password_hash"] = HashPassword(password);
  builder["email"] = email;
  builder["first_name"] = first_name;
  builder["last_name"] = last_name;
  builder["recipe_ids"] = formats::json::MakeArray();
  builder["favorite_recipe_ids"] = formats::json::MakeArray();
  builder["created_at"] = now;

  users.InsertOne(builder.ExtractValue());

  User user;
  user.id = user_id;
  user.username = username;
  user.first_name = first_name;
  user.last_name = last_name;
  user.email = email;
  return user;
}

std::optional<User> MongoStorage::FindUserByUsername(const std::string& username) const {
  auto users = pool_.GetCollection(kUsersCollection);
  auto doc = users.FindOne(formats::json::MakeObject("username", username));

  if (!doc) return std::nullopt;

  User user;
  user.id = (*doc)["_id"].As<std::string>();
  user.username = (*doc)["username"].As<std::string>();
  user.first_name = (*doc)["first_name"].As<std::string>();
  user.last_name = (*doc)["last_name"].As<std::string>();
  user.email = (*doc)["email"].As<std::string>();
  return user;
}

std::vector<User> MongoStorage::SearchUsersByNameMask(
    const std::optional<std::string>& first_name,
    const std::optional<std::string>& last_name
) const {
  if (!first_name && !last_name) {
    throw ApiException(server::http::HttpStatus::kBadRequest, "first_name or last_name is required");
  }

  formats::json::ValueBuilder filter(formats::common::Type::kObject);

  if (first_name) {
    filter["first_name"]["$regex"] = *first_name;
    filter["first_name"]["$options"] = "i";
  }
  if (last_name) {
    filter["last_name"]["$regex"] = *last_name;
    filter["last_name"]["$options"] = "i";
  }

  auto users = pool_.GetCollection(kUsersCollection);
  auto cursor = users.Find(filter.ExtractValue());

  std::vector<User> result;
  for (const auto& doc : cursor) {
    User user;
    user.id = doc["_id"].As<std::string>();
    user.username = doc["username"].As<std::string>();
    user.first_name = doc["first_name"].As<std::string>();
    user.last_name = doc["last_name"].As<std::string>();
    result.push_back(std::move(user));
  }

  std::sort(result.begin(), result.end(), [](const User& lhs, const User& rhs) {
    return lhs.username < rhs.username;
  });

  return result;
}

Recipe MongoStorage::CreateRecipe(
    const std::string& user_id,
    const std::string& title,
    const std::string& description,
    const std::string& instructions,
    int cooking_time_minutes
) {
  if (title.empty() || title.size() > 200) {
    throw ApiException(server::http::HttpStatus::kBadRequest, "Recipe title must be between 1 and 200 characters");
  }
  if (description.size() > 1000) {
    throw ApiException(server::http::HttpStatus::kBadRequest, "Recipe description cannot exceed 1000 characters");
  }
  if (instructions.empty() || instructions.size() > 10000) {
    throw ApiException(server::http::HttpStatus::kBadRequest, "Instructions must be between 1 and 10000 characters");
  }
  if (cooking_time_minutes < 1 || cooking_time_minutes > 10080) {
    throw ApiException(server::http::HttpStatus::kBadRequest, "Cooking time must be between 1 and 10080 minutes");
  }

  // Verify user exists
  auto users = pool_.GetCollection(kUsersCollection);
  auto user_doc = users.FindOne(formats::json::MakeObject("_id", user_id));
  if (!user_doc) {
    throw ApiException(server::http::HttpStatus::kNotFound, "User not found");
  }

  const auto recipe_id = utils::generators::GenerateUuid();
  const auto now = std::chrono::system_clock::now();

  formats::json::ValueBuilder recipe_builder;
  recipe_builder["_id"] = recipe_id;
  recipe_builder["title"] = title;
  recipe_builder["description"] = description;
  recipe_builder["instructions"] = instructions;
  recipe_builder["cooking_time_minutes"] = cooking_time_minutes;
  recipe_builder["author_id"] = user_id;
  recipe_builder["favorite_count"] = 0;
  recipe_builder["created_at"] = now;
  recipe_builder["ingredients"] = formats::json::MakeArray();

  auto recipes = pool_.GetCollection(kRecipesCollection);
  recipes.InsertOne(recipe_builder.ExtractValue());

  // Add recipe_id to user's recipe_ids
  formats::json::ValueBuilder update;
  update["$push"]["recipe_ids"] = recipe_id;
  users.UpdateOne(
      formats::json::MakeObject("_id", user_id),
      update.ExtractValue()
  );

  Recipe recipe;
  recipe.id = recipe_id;
  recipe.user_id = user_id;
  recipe.title = title;
  recipe.description = description;
  recipe.instructions = instructions;
  recipe.cooking_time_minutes = cooking_time_minutes;
  return recipe;
}

std::vector<Recipe> MongoStorage::GetRecipes() const {
  auto recipes = pool_.GetCollection(kRecipesCollection);

  std::vector<Recipe> result;
  auto cursor = recipes.Find({}, storages::mongo::options::Sort{{"title", storages::mongo::options::Sort::kAscending}});

  for (const auto& doc : cursor) {
    Recipe recipe;
    recipe.id = doc["_id"].As<std::string>();
    recipe.user_id = doc["author_id"].As<std::string>();
    recipe.title = doc["title"].As<std::string>();
    recipe.description = doc["description"].As<std::string>("");
    recipe.instructions = doc["instructions"].As<std::string>();
    recipe.cooking_time_minutes = doc["cooking_time_minutes"].As<int>();

    auto ingredients_arr = doc["ingredients"];
    for (const auto& ing : ingredients_arr) {
      Ingredient ingredient;
      ingredient.name = ing["name"].As<std::string>();
      ingredient.amount = ing["amount"].As<std::string>();
      ingredient.unit = ing["unit"].As<std::string>("");
      recipe.ingredients.push_back(std::move(ingredient));
    }

    result.push_back(std::move(recipe));
  }

  return result;
}

std::vector<Recipe> MongoStorage::SearchRecipesByTitle(const std::string& title_mask) const {
  if (title_mask.empty()) {
    throw ApiException(server::http::HttpStatus::kBadRequest, "title is required");
  }

  // Convert wildcard mask to regex
  std::string regex_pattern;
  for (char ch : title_mask) {
    if (ch == '*') {
      regex_pattern += ".*";
    } else if (ch == '.' || ch == '+' || ch == '?' || ch == '^' || ch == '$' ||
               ch == '(' || ch == ')' || ch == '[' || ch == ']' || ch == '{' || ch == '}' || ch == '|' || ch == '\\') {
      regex_pattern += '\\';
      regex_pattern += ch;
    } else {
      regex_pattern += ch;
    }
  }

  formats::json::ValueBuilder filter;
  filter["title"]["$regex"] = regex_pattern;
  filter["title"]["$options"] = "i";

  auto recipes = pool_.GetCollection(kRecipesCollection);
  auto cursor = recipes.Find(
      filter.ExtractValue(),
      storages::mongo::options::Sort{{"title", storages::mongo::options::Sort::kAscending}}
  );

  std::vector<Recipe> result;
  for (const auto& doc : cursor) {
    Recipe recipe;
    recipe.id = doc["_id"].As<std::string>();
    recipe.user_id = doc["author_id"].As<std::string>();
    recipe.title = doc["title"].As<std::string>();
    recipe.description = doc["description"].As<std::string>("");
    recipe.instructions = doc["instructions"].As<std::string>();
    recipe.cooking_time_minutes = doc["cooking_time_minutes"].As<int>();

    for (const auto& ing : doc["ingredients"]) {
      Ingredient ingredient;
      ingredient.name = ing["name"].As<std::string>();
      ingredient.amount = ing["amount"].As<std::string>();
      ingredient.unit = ing["unit"].As<std::string>("");
      recipe.ingredients.push_back(std::move(ingredient));
    }

    result.push_back(std::move(recipe));
  }

  return result;
}

Ingredient MongoStorage::AddIngredientToRecipe(
    const std::string& recipe_id,
    const std::string& name,
    const std::string& amount,
    const std::string& unit
) {
  if (name.empty() || name.size() > 200) {
    throw ApiException(server::http::HttpStatus::kBadRequest, "Ingredient name must be between 1 and 200 characters");
  }
  if (amount.empty() || amount.size() > 100) {
    throw ApiException(server::http::HttpStatus::kBadRequest, "Ingredient amount must be between 1 and 100 characters");
  }
  if (unit.size() > 50) {
    throw ApiException(server::http::HttpStatus::kBadRequest, "Ingredient unit cannot exceed 50 characters");
  }

  auto recipes = pool_.GetCollection(kRecipesCollection);

  const auto ing_id = utils::generators::GenerateUuid();

  formats::json::ValueBuilder ingredient_builder;
  ingredient_builder["id"] = ing_id;
  ingredient_builder["name"] = name;
  ingredient_builder["amount"] = amount;
  ingredient_builder["unit"] = unit;

  formats::json::ValueBuilder update;
  update["$push"]["ingredients"] = ingredient_builder.ExtractValue();

  auto result = recipes.UpdateOne(
      formats::json::MakeObject("_id", recipe_id),
      update.ExtractValue()
  );

  if (result.MatchedCount() == 0) {
    throw ApiException(server::http::HttpStatus::kNotFound, "Recipe not found");
  }

  Ingredient ingredient;
  ingredient.id = ing_id;
  ingredient.name = name;
  ingredient.amount = amount;
  ingredient.unit = unit;
  return ingredient;
}

std::vector<Ingredient> MongoStorage::GetRecipeIngredients(const std::string& recipe_id) const {
  auto recipes = pool_.GetCollection(kRecipesCollection);
  auto doc = recipes.FindOne(formats::json::MakeObject("_id", recipe_id));

  if (!doc) {
    throw ApiException(server::http::HttpStatus::kNotFound, "Recipe not found");
  }

  std::vector<Ingredient> result;
  for (const auto& ing : (*doc)["ingredients"]) {
    Ingredient ingredient;
    ingredient.id = ing["id"].As<std::string>("");
    ingredient.name = ing["name"].As<std::string>();
    ingredient.amount = ing["amount"].As<std::string>();
    ingredient.unit = ing["unit"].As<std::string>("");
    result.push_back(std::move(ingredient));
  }

  std::sort(result.begin(), result.end(), [](const Ingredient& lhs, const Ingredient& rhs) {
    return lhs.name < rhs.name;
  });

  return result;
}

std::vector<Recipe> MongoStorage::GetUserRecipes(const std::string& user_id) const {
  auto recipes = pool_.GetCollection(kRecipesCollection);

  auto cursor = recipes.Find(
      formats::json::MakeObject("author_id", user_id),
      storages::mongo::options::Sort{{"title", storages::mongo::options::Sort::kAscending}}
  );

  std::vector<Recipe> result;
  for (const auto& doc : cursor) {
    Recipe recipe;
    recipe.id = doc["_id"].As<std::string>();
    recipe.user_id = doc["author_id"].As<std::string>();
    recipe.title = doc["title"].As<std::string>();
    recipe.description = doc["description"].As<std::string>("");
    recipe.instructions = doc["instructions"].As<std::string>();
    recipe.cooking_time_minutes = doc["cooking_time_minutes"].As<int>();

    for (const auto& ing : doc["ingredients"]) {
      Ingredient ingredient;
      ingredient.name = ing["name"].As<std::string>();
      ingredient.amount = ing["amount"].As<std::string>();
      ingredient.unit = ing["unit"].As<std::string>("");
      recipe.ingredients.push_back(std::move(ingredient));
    }

    result.push_back(std::move(recipe));
  }

  return result;
}

Recipe MongoStorage::AddFavoriteRecipe(const std::string& user_id, const std::string& recipe_id) {
  auto users = pool_.GetCollection(kUsersCollection);
  auto recipes = pool_.GetCollection(kRecipesCollection);

  // Check user exists
  auto user_doc = users.FindOne(formats::json::MakeObject("_id", user_id));
  if (!user_doc) {
    throw ApiException(server::http::HttpStatus::kNotFound, "User not found");
  }

  // Check recipe exists and add to favorites
  auto recipe_doc = recipes.FindOne(formats::json::MakeObject("_id", recipe_id));
  if (!recipe_doc) {
    throw ApiException(server::http::HttpStatus::kNotFound, "Recipe not found");
  }

  // Add to user's favorites
  formats::json::ValueBuilder update_fav;
  update_fav["$addToSet"]["favorite_recipe_ids"] = recipe_id;
  users.UpdateOne(
      formats::json::MakeObject("_id", user_id),
      update_fav.ExtractValue()
  );

  // Increment favorite count
  formats::json::ValueBuilder update_count;
  update_count["$inc"]["favorite_count"] = 1;
  recipes.UpdateOne(
      formats::json::MakeObject("_id", recipe_id),
      update_count.ExtractValue()
  );

  Recipe recipe;
  recipe.id = recipe_id;
  recipe.user_id = (*recipe_doc)["author_id"].As<std::string>();
  recipe.title = (*recipe_doc)["title"].As<std::string>();
  recipe.description = (*recipe_doc)["description"].As<std::string>("");
  recipe.instructions = (*recipe_doc)["instructions"].As<std::string>();
  recipe.cooking_time_minutes = (*recipe_doc)["cooking_time_minutes"].As<int>();

  return recipe;
}

std::vector<Recipe> MongoStorage::GetFavoriteRecipes(const std::string& user_id) const {
  auto users = pool_.GetCollection(kUsersCollection);
  auto user_doc = users.FindOne(formats::json::MakeObject("_id", user_id));

  if (!user_doc) {
    throw ApiException(server::http::HttpStatus::kNotFound, "User not found");
  }

  // Get favorite recipe IDs
  std::vector<formats::json::Value> fav_ids;
  for (const auto& id_val : (*user_doc)["favorite_recipe_ids"]) {
    fav_ids.push_back(id_val);
  }

  if (fav_ids.empty()) {
    return {};
  }

  // Find recipes by IDs
  formats::json::ValueBuilder ids_array(formats::common::Type::kArray);
  for (const auto& id_val : fav_ids) {
    ids_array.PushBack(id_val);
  }

  formats::json::ValueBuilder filter;
  filter["_id"]["$in"] = ids_array.ExtractValue();

  auto recipes = pool_.GetCollection(kRecipesCollection);
  auto cursor = recipes.Find(
      filter.ExtractValue(),
      storages::mongo::options::Sort{{"title", storages::mongo::options::Sort::kAscending}}
  );

  std::vector<Recipe> result;
  for (const auto& doc : cursor) {
    Recipe recipe;
    recipe.id = doc["_id"].As<std::string>();
    recipe.user_id = doc["author_id"].As<std::string>();
    recipe.title = doc["title"].As<std::string>();
    recipe.description = doc["description"].As<std::string>("");
    recipe.instructions = doc["instructions"].As<std::string>();
    recipe.cooking_time_minutes = doc["cooking_time_minutes"].As<int>();

    for (const auto& ing : doc["ingredients"]) {
      Ingredient ingredient;
      ingredient.name = ing["name"].As<std::string>();
      ingredient.amount = ing["amount"].As<std::string>();
      ingredient.unit = ing["unit"].As<std::string>("");
      recipe.ingredients.push_back(std::move(ingredient));
    }

    result.push_back(std::move(recipe));
  }

  return result;
}

// ========== Validation ==========

void MongoStorage::ValidateUser(
    const std::string& username,
    const std::string& first_name,
    const std::string& last_name,
    const std::string& email,
    const std::string& password
) const {
  if (username.size() < 3 || username.size() > 50) {
    throw ApiException(server::http::HttpStatus::kBadRequest, "Username must be between 3 and 50 characters");
  }
  if (first_name.empty() || first_name.size() > 100) {
    throw ApiException(server::http::HttpStatus::kBadRequest, "First name must be between 1 and 100 characters");
  }
  if (last_name.empty() || last_name.size() > 100) {
    throw ApiException(server::http::HttpStatus::kBadRequest, "Last name must be between 1 and 100 characters");
  }
  if (!IsValidEmail(email)) {
    throw ApiException(server::http::HttpStatus::kBadRequest, "Invalid email");
  }
  if (password.size() < 6) {
    throw ApiException(server::http::HttpStatus::kBadRequest, "Password must be at least 6 characters");
  }
}

// ========== JSON serialization ==========

formats::json::Value MongoStorage::UserToJson(const User& user) const {
  formats::json::ValueBuilder builder(formats::common::Type::kObject);
  builder["id"] = user.id;
  builder["username"] = user.username;
  builder["first_name"] = user.first_name;
  builder["last_name"] = user.last_name;
  builder["email"] = user.email;
  return builder.ExtractValue();
}

formats::json::Value MongoStorage::IngredientToJson(const Ingredient& ingredient) const {
  formats::json::ValueBuilder builder(formats::common::Type::kObject);
  builder["id"] = ingredient.id;
  builder["name"] = ingredient.name;
  builder["amount"] = ingredient.amount;
  builder["unit"] = ingredient.unit;
  return builder.ExtractValue();
}

formats::json::Value MongoStorage::RecipeToJson(const Recipe& recipe) const {
  formats::json::ValueBuilder builder(formats::common::Type::kObject);
  builder["id"] = recipe.id;
  builder["user_id"] = recipe.user_id;
  builder["title"] = recipe.title;
  builder["description"] = recipe.description;
  builder["instructions"] = recipe.instructions;
  builder["cooking_time_minutes"] = recipe.cooking_time_minutes;

  formats::json::ValueBuilder ingredients_builder(formats::common::Type::kArray);
  for (const auto& ingredient : recipe.ingredients) {
    ingredients_builder.PushBack(IngredientToJson(ingredient));
  }
  builder["ingredients"] = ingredients_builder.ExtractValue();

  return builder.ExtractValue();
}

}  // namespace recipe_manager

USERVER_NAMESPACE_END
