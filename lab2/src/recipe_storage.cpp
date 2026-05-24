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

User RecipeStorage::CreateUser(
    const std::string& username,
    const std::string& first_name,
    const std::string& last_name,
    const std::string& email,
    const std::string& password
) {
  ValidateUser(username, first_name, last_name, email, password);

  std::lock_guard lock(mutex_);
  if (user_ids_by_username_.contains(username)) {
    throw ApiException(server::http::HttpStatus::kConflict, "Username already exists");
  }
  for (const auto& [_, user] : users_by_id_) {
    if (user.email == email) {
      throw ApiException(server::http::HttpStatus::kConflict, "Email already exists");
    }
  }

  User user{
      .id = utils::generators::GenerateUuid(),
      .username = username,
      .first_name = first_name,
      .last_name = last_name,
      .email = email,
      .hashed_password = HashPassword(password),
  };
  users_by_id_.emplace(user.id, user);
  user_ids_by_username_[user.username] = user.id;
  return user;
}

std::optional<User> RecipeStorage::FindUserByUsername(const std::string& username) const {
  std::lock_guard lock(mutex_);
  const auto username_it = user_ids_by_username_.find(username);
  if (username_it == user_ids_by_username_.end()) {
    return std::nullopt;
  }
  return users_by_id_.at(username_it->second);
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
  for (const auto& [_, user] : users_by_id_) {
    const bool first_ok = !first_name || WildcardMatch(*first_name, user.first_name);
    const bool last_ok = !last_name || WildcardMatch(*last_name, user.last_name);
    if (first_ok && last_ok) {
      result.push_back(user);
    }
  }
  std::sort(result.begin(), result.end(), [](const User& lhs, const User& rhs) {
    return lhs.username < rhs.username;
  });
  return result;
}

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

  Recipe recipe{
      .id = utils::generators::GenerateUuid(),
      .user_id = user_id,
      .title = title,
      .description = description,
      .instructions = instructions,
      .cooking_time_minutes = cooking_time_minutes,
      .ingredients = {},
  };
  recipes_by_id_.emplace(recipe.id, recipe);
  return recipe;
}

std::vector<Recipe> RecipeStorage::GetRecipes() const {
  std::vector<Recipe> result;
  std::lock_guard lock(mutex_);
  result.reserve(recipes_by_id_.size());
  for (const auto& [_, recipe] : recipes_by_id_) {
    result.push_back(recipe);
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
  for (const auto& [_, recipe] : recipes_by_id_) {
    if (WildcardMatch(title_mask, recipe.title)) {
      result.push_back(recipe);
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
  recipe.ingredients.push_back(ingredient);
  return ingredient;
}

std::vector<Ingredient> RecipeStorage::GetRecipeIngredients(const std::string& recipe_id) const {
  std::lock_guard lock(mutex_);
  return GetRecipeLocked(recipe_id).ingredients;
}

std::vector<Recipe> RecipeStorage::GetUserRecipes(const std::string& user_id) const {
  std::vector<Recipe> result;
  std::lock_guard lock(mutex_);
  EnsureUserExistsLocked(user_id);
  for (const auto& [_, recipe] : recipes_by_id_) {
    if (recipe.user_id == user_id) {
      result.push_back(recipe);
    }
  }
  std::sort(result.begin(), result.end(), [](const Recipe& lhs, const Recipe& rhs) {
    return lhs.title < rhs.title;
  });
  return result;
}

Recipe RecipeStorage::AddFavoriteRecipe(const std::string& user_id, const std::string& recipe_id) {
  std::lock_guard lock(mutex_);
  EnsureUserExistsLocked(user_id);
  const auto& recipe = GetRecipeLocked(recipe_id);
  favorite_recipe_ids_by_user_[user_id].insert(recipe_id);
  return recipe;
}

std::vector<Recipe> RecipeStorage::GetFavoriteRecipes(const std::string& user_id) const {
  std::vector<Recipe> result;
  std::lock_guard lock(mutex_);
  EnsureUserExistsLocked(user_id);
  const auto favorites_it = favorite_recipe_ids_by_user_.find(user_id);
  if (favorites_it == favorite_recipe_ids_by_user_.end()) {
    return result;
  }
  for (const auto& recipe_id : favorites_it->second) {
    result.push_back(GetRecipeLocked(recipe_id));
  }
  std::sort(result.begin(), result.end(), [](const Recipe& lhs, const Recipe& rhs) {
    return lhs.title < rhs.title;
  });
  return result;
}

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

formats::json::Value RecipeStorage::RecipeToJson(const Recipe& recipe, bool include_ingredients) const {
  formats::json::ValueBuilder builder(formats::common::Type::kObject);
  builder["id"] = recipe.id;
  builder["user_id"] = recipe.user_id;
  builder["title"] = recipe.title;
  builder["description"] = recipe.description;
  builder["instructions"] = recipe.instructions;
  builder["cooking_time_minutes"] = recipe.cooking_time_minutes;

  if (include_ingredients) {
    formats::json::ValueBuilder ingredients_builder(formats::common::Type::kArray);
    for (const auto& ingredient : recipe.ingredients) {
      ingredients_builder.PushBack(IngredientToJson(ingredient));
    }
    builder["ingredients"] = ingredients_builder.ExtractValue();
  }

  return builder.ExtractValue();
}

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
  if (!users_by_id_.contains(user_id)) {
    throw ApiException(server::http::HttpStatus::kNotFound, "User not found");
  }
}

Recipe& RecipeStorage::GetRecipeLocked(const std::string& recipe_id) {
  const auto recipe_it = recipes_by_id_.find(recipe_id);
  if (recipe_it == recipes_by_id_.end()) {
    throw ApiException(server::http::HttpStatus::kNotFound, "Recipe not found");
  }
  return recipe_it->second;
}

const Recipe& RecipeStorage::GetRecipeLocked(const std::string& recipe_id) const {
  const auto recipe_it = recipes_by_id_.find(recipe_id);
  if (recipe_it == recipes_by_id_.end()) {
    throw ApiException(server::http::HttpStatus::kNotFound, "Recipe not found");
  }
  return recipe_it->second;
}

}  // namespace recipe_manager

USERVER_NAMESPACE_END
