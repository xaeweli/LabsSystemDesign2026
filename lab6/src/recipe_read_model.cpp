#include "recipe_read_model.hpp"

#include <algorithm>

#include <userver/formats/json/value_builder.hpp>

#include "api_utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace recipe_manager {

RecipeReadModel::RecipeReadModel(const components::ComponentConfig& config, const components::ComponentContext& context)
    : LoggableComponentBase(config, context) {}

void RecipeReadModel::OnUserCreated(const std::string& user_id, const std::string& username) {
  std::lock_guard lock(mutex_);
  users_[user_id] = {user_id, username};
}

void RecipeReadModel::OnRecipeCreated(
    const std::string& recipe_id,
    const std::string& user_id,
    const std::string& title,
    int cooking_time_minutes
) {
  std::lock_guard lock(mutex_);
  recipes_[recipe_id] = {
      .id = recipe_id,
      .user_id = user_id,
      .title = title,
      .cooking_time_minutes = cooking_time_minutes,
      .ingredient_count = 0,
  };
  user_recipe_ids_[user_id].insert(recipe_id);
}

void RecipeReadModel::OnIngredientAdded(
    const std::string& recipe_id,
    const std::string& ingredient_id,
    const std::string& name,
    const std::string& amount,
    const std::string& unit
) {
  std::lock_guard lock(mutex_);
  auto it = recipes_.find(recipe_id);
  if (it != recipes_.end()) {
    it->second.ingredient_count++;
  }
}

void RecipeReadModel::OnRecipeFavorited(const std::string& user_id, const std::string& recipe_id) {
  std::lock_guard lock(mutex_);
  user_favorite_ids_[user_id].insert(recipe_id);
}

std::vector<RecipeSummary> RecipeReadModel::GetAllRecipes() const {
  std::vector<RecipeSummary> result;
  std::lock_guard lock(mutex_);
  result.reserve(recipes_.size());
  for (const auto& [_, summary] : recipes_) {
    result.push_back(summary);
  }
  std::sort(result.begin(), result.end(), [](const RecipeSummary& lhs, const RecipeSummary& rhs) {
    return lhs.title < rhs.title;
  });
  return result;
}

std::vector<RecipeSummary> RecipeReadModel::SearchRecipes(const std::string& title_mask) const {
  std::vector<RecipeSummary> result;
  std::lock_guard lock(mutex_);
  for (const auto& [_, summary] : recipes_) {
    if (WildcardMatch(title_mask, summary.title)) {
      result.push_back(summary);
    }
  }
  std::sort(result.begin(), result.end(), [](const RecipeSummary& lhs, const RecipeSummary& rhs) {
    return lhs.title < rhs.title;
  });
  return result;
}

std::optional<RecipeSummary> RecipeReadModel::GetRecipeSummary(const std::string& recipe_id) const {
  std::lock_guard lock(mutex_);
  auto it = recipes_.find(recipe_id);
  if (it == recipes_.end()) return std::nullopt;
  return it->second;
}

std::vector<RecipeSummary> RecipeReadModel::GetUserRecipes(const std::string& user_id) const {
  std::vector<RecipeSummary> result;
  std::lock_guard lock(mutex_);
  auto user_it = user_recipe_ids_.find(user_id);
  if (user_it == user_recipe_ids_.end()) return result;
  for (const auto& recipe_id : user_it->second) {
    auto it = recipes_.find(recipe_id);
    if (it != recipes_.end()) {
      result.push_back(it->second);
    }
  }
  std::sort(result.begin(), result.end(), [](const RecipeSummary& lhs, const RecipeSummary& rhs) {
    return lhs.title < rhs.title;
  });
  return result;
}

std::vector<RecipeSummary> RecipeReadModel::GetFavoriteRecipes(const std::string& user_id) const {
  std::vector<RecipeSummary> result;
  std::lock_guard lock(mutex_);
  auto fav_it = user_favorite_ids_.find(user_id);
  if (fav_it == user_favorite_ids_.end()) return result;
  for (const auto& recipe_id : fav_it->second) {
    auto it = recipes_.find(recipe_id);
    if (it != recipes_.end()) {
      result.push_back(it->second);
    }
  }
  std::sort(result.begin(), result.end(), [](const RecipeSummary& lhs, const RecipeSummary& rhs) {
    return lhs.title < rhs.title;
  });
  return result;
}

formats::json::Value RecipeReadModel::RecipeSummaryToJson(const RecipeSummary& summary) const {
  formats::json::ValueBuilder builder(formats::common::Type::kObject);
  builder["id"] = summary.id;
  builder["user_id"] = summary.user_id;
  builder["title"] = summary.title;
  builder["cooking_time_minutes"] = summary.cooking_time_minutes;
  builder["ingredient_count"] = summary.ingredient_count;
  return builder.ExtractValue();
}

}  // namespace recipe_manager

USERVER_NAMESPACE_END
