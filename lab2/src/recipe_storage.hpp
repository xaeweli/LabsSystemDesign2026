#pragma once

#include <mutex>
#include <optional>
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

class RecipeStorage final : public components::LoggableComponentBase {
 public:
  static constexpr std::string_view kName = "recipe-storage";

  RecipeStorage(const components::ComponentConfig& config, const components::ComponentContext& context);

  User CreateUser(
      const std::string& username,
      const std::string& first_name,
      const std::string& last_name,
      const std::string& email,
      const std::string& password
  );

  std::optional<User> FindUserByUsername(const std::string& username) const;
  std::vector<User> SearchUsersByNameMask(
      const std::optional<std::string>& first_name,
      const std::optional<std::string>& last_name
  ) const;

  Recipe CreateRecipe(
      const std::string& user_id,
      const std::string& title,
      const std::string& description,
      const std::string& instructions,
      int cooking_time_minutes
  );
  std::vector<Recipe> GetRecipes() const;
  std::vector<Recipe> SearchRecipesByTitle(const std::string& title_mask) const;
  Ingredient AddIngredientToRecipe(
      const std::string& recipe_id,
      const std::string& name,
      const std::string& amount,
      const std::string& unit
  );
  std::vector<Ingredient> GetRecipeIngredients(const std::string& recipe_id) const;
  std::vector<Recipe> GetUserRecipes(const std::string& user_id) const;
  Recipe AddFavoriteRecipe(const std::string& user_id, const std::string& recipe_id);
  std::vector<Recipe> GetFavoriteRecipes(const std::string& user_id) const;

  formats::json::Value UserToJson(const User& user) const;
  formats::json::Value IngredientToJson(const Ingredient& ingredient) const;
  formats::json::Value RecipeToJson(const Recipe& recipe, bool include_ingredients = true) const;

 private:
  void ValidateUser(
      const std::string& username,
      const std::string& first_name,
      const std::string& last_name,
      const std::string& email,
      const std::string& password
  ) const;
  void EnsureUserExistsLocked(const std::string& user_id) const;
  Recipe& GetRecipeLocked(const std::string& recipe_id);
  const Recipe& GetRecipeLocked(const std::string& recipe_id) const;

  mutable std::mutex mutex_;
  std::unordered_map<std::string, User> users_by_id_;
  std::unordered_map<std::string, std::string> user_ids_by_username_;
  std::unordered_map<std::string, Recipe> recipes_by_id_;
  std::unordered_map<std::string, std::unordered_set<std::string>> favorite_recipe_ids_by_user_;
};

}  // namespace recipe_manager

USERVER_NAMESPACE_END
