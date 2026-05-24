#pragma once

#include <memory>
#include <string>
#include <string_view>

#include <userver/components/loggable_component_base.hpp>
#include <amqp.h>

#include "event_types.hpp"

USERVER_NAMESPACE_BEGIN

namespace recipe_manager {

class EventProducer final : public components::LoggableComponentBase {
 public:
  static constexpr std::string_view kName = "event-producer";

  EventProducer(const components::ComponentConfig& config, const components::ComponentContext& context);
  ~EventProducer() override;

  void Publish(const EventEnvelope& event) const;

 private:
  void Connect();
  void Disconnect();

  mutable amqp_connection_state_t connection_;
  mutable amqp_channel_t channel_{1};
  mutable amqp_bytes_t exchange_;
  mutable bool connected_{false};

  std::string host_;
  int port_;
  std::string exchange_name_;
};

}  // namespace recipe_manager

USERVER_NAMESPACE_END
