#include "event_types.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <userver/utils/uuid4.hpp>

USERVER_NAMESPACE_BEGIN

namespace recipe_manager {

namespace {

std::string MakeEventId() {
  return utils::generators::GenerateUuid();
}

}  // namespace

std::string CurrentTimestampIso() {
  const auto now = std::chrono::system_clock::now();
  const auto now_time_t = std::chrono::system_clock::to_time_t(now);
  const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch()
  ).count() % 1000;

  std::tm tm{};
#ifdef _WIN32
  gmtime_s(&tm, &now_time_t);
#else
  gmtime_r(&now_time_t, &tm);
#endif

  std::ostringstream out;
  out << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S")
      << '.' << std::setfill('0') << std::setw(3) << ms << 'Z';
  return out.str();
}

EventEnvelope MakeUserCreatedEvent(
    const std::string& event_id,
    const std::string& user_id,
    const std::string& username,
    const std::string& email
) {
  formats::json::ValueBuilder payload(formats::common::Type::kObject);
  payload["user_id"] = user_id;
  payload["username"] = username;
  payload["email"] = email;

  return EventEnvelope{
      .event_id = event_id.empty() ? MakeEventId() : event_id,
      .event_type = EventType::kUserCreated,
      .timestamp = CurrentTimestampIso(),
      .payload = payload.ExtractValue(),
  };
}

EventEnvelope MakeRecipeCreatedEvent(
    const std::string& event_id,
    const std::string& recipe_id,
    const std::string& user_id,
    const std::string& title,
    int cooking_time_minutes
) {
  formats::json::ValueBuilder payload(formats::common::Type::kObject);
  payload["recipe_id"] = recipe_id;
  payload["user_id"] = user_id;
  payload["title"] = title;
  payload["cooking_time_minutes"] = cooking_time_minutes;

  return EventEnvelope{
      .event_id = event_id.empty() ? MakeEventId() : event_id,
      .event_type = EventType::kRecipeCreated,
      .timestamp = CurrentTimestampIso(),
      .payload = payload.ExtractValue(),
  };
}

EventEnvelope MakeIngredientAddedEvent(
    const std::string& event_id,
    const std::string& recipe_id,
    const std::string& ingredient_id,
    const std::string& name,
    const std::string& amount,
    const std::string& unit
) {
  formats::json::ValueBuilder payload(formats::common::Type::kObject);
  payload["recipe_id"] = recipe_id;
  payload["ingredient_id"] = ingredient_id;
  payload["name"] = name;
  payload["amount"] = amount;
  payload["unit"] = unit;

  return EventEnvelope{
      .event_id = event_id.empty() ? MakeEventId() : event_id,
      .event_type = EventType::kIngredientAdded,
      .timestamp = CurrentTimestampIso(),
      .payload = payload.ExtractValue(),
  };
}

EventEnvelope MakeRecipeFavoritedEvent(
    const std::string& event_id,
    const std::string& user_id,
    const std::string& recipe_id
) {
  formats::json::ValueBuilder payload(formats::common::Type::kObject);
  payload["user_id"] = user_id;
  payload["recipe_id"] = recipe_id;

  return EventEnvelope{
      .event_id = event_id.empty() ? MakeEventId() : event_id,
      .event_type = EventType::kRecipeFavorited,
      .timestamp = CurrentTimestampIso(),
      .payload = payload.ExtractValue(),
  };
}

std::string RoutingKeyForEvent(EventType type) {
  switch (type) {
    case EventType::kUserCreated: return "user.created";
    case EventType::kRecipeCreated: return "recipe.created";
    case EventType::kIngredientAdded: return "recipe.ingredient.added";
    case EventType::kRecipeFavorited: return "recipe.favorited";
  }
  return "unknown";
}

}  // namespace recipe_manager

USERVER_NAMESPACE_END
