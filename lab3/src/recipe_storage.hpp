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

// Хранилище с архитектурой 3 БД (Recipes DB, Users DB, Auth DB),
// реализованное in-memory для совместимости с userver без PostgreSQL.
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
  formats::json::Value RecipeToJson(const Recipe& recipe) const;

 private:
  // --- Recipes Database ---
  struct RecipeEntry {
    Recipe recipe;
    int favorite_count{0};
  };
  std::unordered_map<std::string, RecipeEntry> recipes_;

  // --- Users Database ---
  struct UserProfile {
    std::string id;
    std::string username;
    std::string first_name;
    std::string last_name;
  };
  std::unordered_map<std::string, UserProfile> profiles_;
  std::unordered_map<std::string, std::string> username_to_id_;
  // user_recipes: user_id -> set of recipe_ids (созданные рецепты)
  std::unordered_map<std::string, std::unordered_set<std::string>> user_recipes_;
  // user_favorites: user_id -> set of recipe_ids (избранные)
  std::unordered_map<std::string, std::unordered_set<std::string>> user_favorites_;

  // --- Auth Database ---
  struct Credentials {
    std::string id;
    std::string login;
    std::string password_hash;
    std::string email;
    std::string facebook_id;
    std::string google_id;
  };
  std::unordered_map<std::string, Credentials> credentials_by_id_;
  std::unordered_map<std::string, std::string> login_to_id_;

  void ValidateUser(
      const std::string& username,
      const std::string& first_name,
      const std::string& last_name,
      const std::string& email,
      const std::string& password
  ) const;

  void EnsureUserExistsLocked(const std::string& user_id) const;
  RecipeEntry& GetRecipeLocked(const std::string& recipe_id);
  const RecipeEntry& GetRecipeLocked(const std::string& recipe_id) const;

  mutable std::mutex mutex_;
};

}  // namespace recipe_manager

USERVER_NAMESPACE_END
