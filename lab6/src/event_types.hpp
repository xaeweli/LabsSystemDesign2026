#pragma once

#include <chrono>
#include <string>
#include <string_view>

#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace recipe_manager {

enum class EventType {
  kUserCreated,
  kRecipeCreated,
  kIngredientAdded,
  kRecipeFavorited,
};

constexpr std::string_view EventTypeToString(EventType type) {
  switch (type) {
    case EventType::kUserCreated: return "UserCreated";
    case EventType::kRecipeCreated: return "RecipeCreated";
    case EventType::kIngredientAdded: return "IngredientAdded";
    case EventType::kRecipeFavorited: return "RecipeFavorited";
  }
  return "Unknown";
}

struct EventEnvelope {
  std::string event_id;
  EventType event_type;
  std::string timestamp;
  formats::json::Value payload;

  formats::json::Value ToJson() const {
    formats::json::ValueBuilder builder(formats::common::Type::kObject);
    builder["event_type"] = std::string{EventTypeToString(event_type)};
    builder["event_id"] = event_id;
    builder["timestamp"] = timestamp;
    builder["payload"] = payload;
    return builder.ExtractValue();
  }
};

EventEnvelope MakeUserCreatedEvent(
    const std::string& event_id,
    const std::string& user_id,
    const std::string& username,
    const std::string& email
);

EventEnvelope MakeRecipeCreatedEvent(
    const std::string& event_id,
    const std::string& recipe_id,
    const std::string& user_id,
    const std::string& title,
    int cooking_time_minutes
);

EventEnvelope MakeIngredientAddedEvent(
    const std::string& event_id,
    const std::string& recipe_id,
    const std::string& ingredient_id,
    const std::string& name,
    const std::string& amount,
    const std::string& unit
);

EventEnvelope MakeRecipeFavoritedEvent(
    const std::string& event_id,
    const std::string& user_id,
    const std::string& recipe_id
);

std::string RoutingKeyForEvent(EventType type);
std::string CurrentTimestampIso();

}  // namespace recipe_manager

USERVER_NAMESPACE_END
