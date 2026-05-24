#pragma once

#include <string>
#include <vector>

#include <userver/components/component.hpp>

USERVER_NAMESPACE_BEGIN

namespace recipe_manager {

struct User {
  std::string id;
  std::string username;
  std::string first_name;
  std::string last_name;
  std::string email;
  std::string hashed_password;
};

struct Ingredient {
  std::string id;
  std::string name;
  std::string amount;
  std::string unit;
};

struct Recipe {
  std::string id;
  std::string user_id;
  std::string title;
  std::string description;
  std::string instructions;
  int cooking_time_minutes;
  std::vector<Ingredient> ingredients;
};

}  // namespace recipe_manager

USERVER_NAMESPACE_END
