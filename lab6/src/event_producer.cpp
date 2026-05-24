#include "event_producer.hpp"

#include <cstdlib>

#include <userver/components/component_context.hpp>
#include <userver/formats/json.hpp>
#include <userver/logging/log.hpp>

#include <amqp_tcp_socket.h>

USERVER_NAMESPACE_BEGIN

namespace recipe_manager {

EventProducer::EventProducer(const components::ComponentConfig& config, const components::ComponentContext& context)
    : LoggableComponentBase(config, context) {
  host_ = config["rabbitmq-host"].As<std::string>("rabbitmq");
  port_ = config["rabbitmq-port"].As<int>(5672);
  exchange_name_ = config["rabbitmq-exchange"].As<std::string>("recipe.events");

  exchange_ = amqp_cstring_bytes(exchange_name_.c_str());

  Connect();
}

EventProducer::~EventProducer() {
  Disconnect();
}

void EventProducer::Connect() {
  connection_ = amqp_new_connection();
  auto* socket = amqp_tcp_socket_new(connection_);
  if (!socket) {
    LOG_ERROR() << "Failed to create TCP socket for RabbitMQ";
    return;
  }

  auto status = amqp_socket_open(socket, host_.c_str(), port_);
  if (status != AMQP_STATUS_OK) {
    LOG_ERROR() << "Failed to connect to RabbitMQ at " << host_ << ":" << port_
                << " (status=" << status << ")";
    amqp_destroy_connection(connection_);
    return;
  }

  auto login = amqp_login(connection_, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, "guest", "guest");
  if (login.reply_type != AMQP_RESPONSE_NORMAL) {
    LOG_ERROR() << "RabbitMQ login failed";
    amqp_destroy_connection(connection_);
    return;
  }

  amqp_channel_open(connection_, channel_);
  auto open_ok = amqp_get_rpc_reply(connection_);
  if (open_ok.reply_type != AMQP_RESPONSE_NORMAL) {
    LOG_ERROR() << "Failed to open RabbitMQ channel";
    amqp_destroy_connection(connection_);
    return;
  }

  amqp_exchange_declare(
      connection_, channel_, exchange_,
      amqp_cstring_bytes("topic"),
      0, 1, 0, 0, amqp_empty_table
  );
  auto exchange_ok = amqp_get_rpc_reply(connection_);
  if (exchange_ok.reply_type != AMQP_RESPONSE_NORMAL) {
    LOG_ERROR() << "Failed to declare RabbitMQ exchange";
    amqp_destroy_connection(connection_);
    return;
  }

  connected_ = true;
  LOG_INFO() << "Connected to RabbitMQ at " << host_ << ":" << port_
             << ", exchange: " << exchange_name_;
}

void EventProducer::Disconnect() {
  if (connected_) {
    amqp_channel_close(connection_, channel_, AMQP_REPLY_SUCCESS);
    amqp_connection_close(connection_, AMQP_REPLY_SUCCESS);
    amqp_destroy_connection(connection_);
    connected_ = false;
  }
}

void EventProducer::Publish(const EventEnvelope& event) const {
  if (!connected_) {
    LOG_WARNING() << "Cannot publish event: not connected to RabbitMQ";
    return;
  }

  const auto body = formats::json::ToString(event.ToJson());
  const auto routing_key = RoutingKeyForEvent(event.event_type);
  const auto routing_key_bytes = amqp_cstring_bytes(routing_key.c_str());
  const auto body_bytes = amqp_cstring_bytes(body.c_str());

  amqp_basic_properties_t props{};
  props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
  props.content_type = amqp_cstring_bytes("application/json");
  props.delivery_mode = 2;

  auto status = amqp_basic_publish(
      connection_, channel_, exchange_,
      routing_key_bytes, 0, 0,
      &props, body_bytes
  );

  if (status != AMQP_STATUS_OK) {
    LOG_ERROR() << "Failed to publish event " << EventTypeToString(event.event_type)
                << " (routing_key=" << routing_key << ")";
  } else {
    LOG_INFO() << "Published event " << EventTypeToString(event.event_type)
               << " (routing_key=" << routing_key << ")";
  }
}

}  // namespace recipe_manager

USERVER_NAMESPACE_END
