#pragma once

#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <userver/components/loggable_component_base.hpp>
#include <userver/formats/json/value.hpp>

#include "api_types.hpp"

USERVER_NAMESPACE_BEGIN

namespace recipe_manager {

struct RecipeSummary {
  std::string id;
  std::string user_id;
  std::string title;
  int cooking_time_minutes{0};
  int ingredient_count{0};
};

struct UserProfileView {
  std::string id;
  std::string username;
};

class RecipeReadModel final : public components::LoggableComponentBase {
 public:
  static constexpr std::string_view kName = "recipe-read-model";

  RecipeReadModel(const components::ComponentConfig& config, const components::ComponentContext& context);

  void OnUserCreated(const std::string& user_id, const std::string& username);
  void OnRecipeCreated(
      const std::string& recipe_id,
      const std::string& user_id,
      const std::string& title,
      int cooking_time_minutes
  );
  void OnIngredientAdded(
      const std::string& recipe_id,
      const std::string& ingredient_id,
      const std::string& name,
      const std::string& amount,
      const std::string& unit
  );
  void OnRecipeFavorited(const std::string& user_id, const std::string& recipe_id);

  std::vector<RecipeSummary> GetAllRecipes() const;
  std::vector<RecipeSummary> SearchRecipes(const std::string& title_mask) const;
  std::optional<RecipeSummary> GetRecipeSummary(const std::string& recipe_id) const;
  std::vector<RecipeSummary> GetUserRecipes(const std::string& user_id) const;
  std::vector<RecipeSummary> GetFavoriteRecipes(const std::string& user_id) const;

  formats::json::Value RecipeSummaryToJson(const RecipeSummary& summary) const;

 private:
  mutable std::mutex mutex_;
  std::unordered_map<std::string, RecipeSummary> recipes_;
  std::unordered_map<std::string, std::unordered_set<std::string>> user_recipe_ids_;
  std::unordered_map<std::string, std::unordered_set<std::string>> user_favorite_ids_;
  std::unordered_map<std::string, UserProfileView> users_;
};

}  // namespace recipe_manager

USERVER_NAMESPACE_END
