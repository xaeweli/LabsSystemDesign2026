#include "recipe_storage.hpp"

#include <algorithm>

#include <userver/formats/json/value_builder.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/utils/uuid4.hpp>

#include "api_utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace recipe_manager {

RecipeStorage::RecipeStorage(const components::ComponentConfig& config, const components::ComponentContext& context)
    : LoggableComponentBase(config, context) {}

// ========== Recipes Database: операции с рецептами ==========

Recipe RecipeStorage::CreateRecipe(
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

  std::lock_guard lock(mutex_);
  EnsureUserExistsLocked(user_id);

  const auto recipe_id = utils::generators::GenerateUuid();

  Recipe recipe{
    .id = recipe_id,
    .user_id = user_id,
    .title = title,
    .description = description,
    .instructions = instructions,
    .cooking_time_minutes = cooking_time_minutes,
    .ingredients = {},
  };

  // Recipes DB: сохраняем рецепт
  recipes_.emplace(recipe_id, RecipeEntry{recipe, 0});
  // Users DB: добавляем в список рецептов пользователя
  user_recipes_[user_id].insert(recipe_id);

  return recipe;
}

std::vector<Recipe> RecipeStorage::GetRecipes() const {
  std::vector<Recipe> result;
  std::lock_guard lock(mutex_);
  result.reserve(recipes_.size());
  for (const auto& [_, entry] : recipes_) {
    result.push_back(entry.recipe);
  }
  std::sort(result.begin(), result.end(), [](const Recipe& lhs, const Recipe& rhs) {
    return lhs.title < rhs.title;
  });
  return result;
}

std::vector<Recipe> RecipeStorage::SearchRecipesByTitle(const std::string& title_mask) const {
  if (title_mask.empty()) {
    throw ApiException(server::http::HttpStatus::kBadRequest, "title is required");
  }

  std::vector<Recipe> result;
  std::lock_guard lock(mutex_);
  for (const auto& [_, entry] : recipes_) {
    if (WildcardMatch(title_mask, entry.recipe.title)) {
      result.push_back(entry.recipe);
    }
  }
  std::sort(result.begin(), result.end(), [](const Recipe& lhs, const Recipe& rhs) {
    return lhs.title < rhs.title;
  });
  return result;
}

Ingredient RecipeStorage::AddIngredientToRecipe(
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

  std::lock_guard lock(mutex_);
  auto& recipe = GetRecipeLocked(recipe_id);
  Ingredient ingredient{
    .id = utils::generators::GenerateUuid(),
    .name = name,
    .amount = amount,
    .unit = unit,
  };
  recipe.recipe.ingredients.push_back(ingredient);
  return ingredient;
}

std::vector<Ingredient> RecipeStorage::GetRecipeIngredients(const std::string& recipe_id) const {
  std::lock_guard lock(mutex_);
  return GetRecipeLocked(recipe_id).recipe.ingredients;
}

std::vector<Recipe> RecipeStorage::GetUserRecipes(const std::string& user_id) const {
  std::vector<Recipe> result;
  std::lock_guard lock(mutex_);
  EnsureUserExistsLocked(user_id);
  for (const auto& [_, entry] : recipes_) {
    if (entry.recipe.user_id == user_id) {
      result.push_back(entry.recipe);
    }
  }
  std::sort(result.begin(), result.end(), [](const Recipe& lhs, const Recipe& rhs) {
    return lhs.title < rhs.title;
  });
  return result;
}

// ========== Users Database: избранное ==========

Recipe RecipeStorage::AddFavoriteRecipe(const std::string& user_id, const std::string& recipe_id) {
  std::lock_guard lock(mutex_);
  EnsureUserExistsLocked(user_id);
  auto& entry = GetRecipeLocked(recipe_id);
  // Users DB: добавляем в избранное
  user_favorites_[user_id].insert(recipe_id);
  // Recipes DB: увеличиваем счётчик
  entry.favorite_count++;
  return entry.recipe;
}

std::vector<Recipe> RecipeStorage::GetFavoriteRecipes(const std::string& user_id) const {
  std::vector<Recipe> result;
  std::lock_guard lock(mutex_);
  EnsureUserExistsLocked(user_id);
  const auto fav_it = user_favorites_.find(user_id);
  if (fav_it == user_favorites_.end()) {
    return result;
  }
  for (const auto& recipe_id : fav_it->second) {
    result.push_back(GetRecipeLocked(recipe_id).recipe);
  }
  std::sort(result.begin(), result.end(), [](const Recipe& lhs, const Recipe& rhs) {
    return lhs.title < rhs.title;
  });
  return result;
}

// ========== Users Database + Auth Database: пользователи ==========

User RecipeStorage::CreateUser(
    const std::string& username,
    const std::string& first_name,
    const std::string& last_name,
    const std::string& email,
    const std::string& password
) {
  ValidateUser(username, first_name, last_name, email, password);

  std::lock_guard lock(mutex_);

  // Auth DB: проверка уникальности логина
  if (login_to_id_.contains(username)) {
    throw ApiException(server::http::HttpStatus::kConflict, "Username already exists");
  }
  // Auth DB: проверка уникальности email
  for (const auto& [_, cred] : credentials_by_id_) {
    if (cred.email == email) {
      throw ApiException(server::http::HttpStatus::kConflict, "Email already exists");
    }
  }

  const auto user_id = utils::generators::GenerateUuid();

  // Auth DB: сохраняем учётные данные
  credentials_by_id_.emplace(user_id, Credentials{
    user_id, username, HashPassword(password), email, "", ""
  });
  login_to_id_[username] = user_id;

  // Users DB: сохраняем профиль
  profiles_.emplace(user_id, UserProfile{user_id, username, first_name, last_name});
  username_to_id_[username] = user_id;

  User user;
  user.id = user_id;
  user.username = username;
  user.first_name = first_name;
  user.last_name = last_name;
  user.email = email;
  return user;
}

std::optional<User> RecipeStorage::FindUserByUsername(const std::string& username) const {
  std::lock_guard lock(mutex_);

  // Auth DB: поиск логина
  const auto login_it = login_to_id_.find(username);
  if (login_it == login_to_id_.end()) {
    return std::nullopt;
  }

  const auto& cred = credentials_by_id_.at(login_it->second);
  // Users DB: получение профиля
  const auto& profile = profiles_.at(cred.id);

  User user;
  user.id = cred.id;
  user.username = cred.login;
  user.first_name = profile.first_name;
  user.last_name = profile.last_name;
  user.email = cred.email;
  return user;
}

std::vector<User> RecipeStorage::SearchUsersByNameMask(
    const std::optional<std::string>& first_name,
    const std::optional<std::string>& last_name
) const {
  if (!first_name && !last_name) {
    throw ApiException(server::http::HttpStatus::kBadRequest, "first_name or last_name is required");
  }

  std::vector<User> result;
  std::lock_guard lock(mutex_);
  for (const auto& [_, profile] : profiles_) {
    const bool first_ok = !first_name || WildcardMatch(*first_name, profile.first_name);
    const bool last_ok = !last_name || WildcardMatch(*last_name, profile.last_name);
    if (first_ok && last_ok) {
      User user;
      user.id = profile.id;
      user.username = profile.username;
      user.first_name = profile.first_name;
      user.last_name = profile.last_name;
      result.push_back(std::move(user));
    }
  }
  std::sort(result.begin(), result.end(), [](const User& lhs, const User& rhs) {
    return lhs.username < rhs.username;
  });
  return result;
}

// ========== Валидация ==========

void RecipeStorage::ValidateUser(
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

void RecipeStorage::EnsureUserExistsLocked(const std::string& user_id) const {
  if (!profiles_.contains(user_id)) {
    throw ApiException(server::http::HttpStatus::kNotFound, "User not found");
  }
}

RecipeStorage::RecipeEntry& RecipeStorage::GetRecipeLocked(const std::string& recipe_id) {
  const auto it = recipes_.find(recipe_id);
  if (it == recipes_.end()) {
    throw ApiException(server::http::HttpStatus::kNotFound, "Recipe not found");
  }
  return it->second;
}

const RecipeStorage::RecipeEntry& RecipeStorage::GetRecipeLocked(const std::string& recipe_id) const {
  const auto it = recipes_.find(recipe_id);
  if (it == recipes_.end()) {
    throw ApiException(server::http::HttpStatus::kNotFound, "Recipe not found");
  }
  return it->second;
}

// ========== JSON сериализация ==========

formats::json::Value RecipeStorage::UserToJson(const User& user) const {
  formats::json::ValueBuilder builder(formats::common::Type::kObject);
  builder["id"] = user.id;
  builder["username"] = user.username;
  builder["first_name"] = user.first_name;
  builder["last_name"] = user.last_name;
  builder["email"] = user.email;
  return builder.ExtractValue();
}

formats::json::Value RecipeStorage::IngredientToJson(const Ingredient& ingredient) const {
  formats::json::ValueBuilder builder(formats::common::Type::kObject);
  builder["id"] = ingredient.id;
  builder["name"] = ingredient.name;
  builder["amount"] = ingredient.amount;
  builder["unit"] = ingredient.unit;
  return builder.ExtractValue();
}

formats::json::Value RecipeStorage::RecipeToJson(const Recipe& recipe) const {
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
