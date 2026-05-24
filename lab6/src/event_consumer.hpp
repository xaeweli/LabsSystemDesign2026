#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <string_view>
#include <thread>

#include <userver/components/loggable_component_base.hpp>
#include <amqp.h>

#include "event_types.hpp"
#include "recipe_read_model.hpp"

USERVER_NAMESPACE_BEGIN

namespace recipe_manager {

class EventConsumer final : public components::LoggableComponentBase {
 public:
  static constexpr std::string_view kName = "event-consumer";

  EventConsumer(const components::ComponentConfig& config, const components::ComponentContext& context);
  ~EventConsumer() override;

 private:
  void Connect();
  void Disconnect();
  void ConsumeLoop();
  void ProcessEvent(const std::string& routing_key, const std::string& body);
  void HandleUserCreated(const formats::json::Value& payload);
  void HandleRecipeCreated(const formats::json::Value& payload);
  void HandleIngredientAdded(const formats::json::Value& payload);
  void HandleRecipeFavorited(const formats::json::Value& payload);

  amqp_connection_state_t connection_;
  amqp_channel_t channel_{1};
  std::atomic<bool> connected_{false};
  std::atomic<bool> running_{false};

  std::string host_;
  int port_;
  std::string queue_name_;
  std::string exchange_name_;

  RecipeReadModel* read_model_{nullptr};

  std::thread consumer_thread_;
};

}  // namespace recipe_manager

USERVER_NAMESPACE_END
