#include "event_consumer.hpp"

#include <chrono>
#include <thread>

#include <userver/components/component_context.hpp>
#include <userver/formats/json.hpp>
#include <userver/logging/log.hpp>

#include <amqp_tcp_socket.h>

USERVER_NAMESPACE_BEGIN

namespace recipe_manager {

EventConsumer::EventConsumer(const components::ComponentConfig& config, const components::ComponentContext& context)
    : LoggableComponentBase(config, context) {
  host_ = config["rabbitmq-host"].As<std::string>("rabbitmq");
  port_ = config["rabbitmq-port"].As<int>(5672);
  exchange_name_ = config["rabbitmq-exchange"].As<std::string>("recipe.events");
  queue_name_ = config["rabbitmq-queue"].As<std::string>("recipe.read-model");

  read_model_ = &context.FindComponent<RecipeReadModel>();

  Connect();
  if (connected_) {
    running_ = true;
    consumer_thread_ = std::thread(&EventConsumer::ConsumeLoop, this);
  }
}

EventConsumer::~EventConsumer() {
  running_ = false;
  if (consumer_thread_.joinable()) {
    consumer_thread_.join();
  }
  Disconnect();
}

void EventConsumer::Connect() {
  connection_ = amqp_new_connection();
  auto* socket = amqp_tcp_socket_new(connection_);
  if (!socket) {
    LOG_ERROR() << "Failed to create TCP socket for consumer";
    return;
  }

  auto status = amqp_socket_open(socket, host_.c_str(), port_);
  if (status != AMQP_STATUS_OK) {
    LOG_ERROR() << "Consumer: failed to connect to RabbitMQ at " << host_ << ":" << port_;
    amqp_destroy_connection(connection_);
    return;
  }

  auto login = amqp_login(connection_, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, "guest", "guest");
  if (login.reply_type != AMQP_RESPONSE_NORMAL) {
    LOG_ERROR() << "Consumer: RabbitMQ login failed";
    amqp_destroy_connection(connection_);
    return;
  }

  amqp_channel_open(connection_, channel_);
  if (amqp_get_rpc_reply(connection_).reply_type != AMQP_RESPONSE_NORMAL) {
    LOG_ERROR() << "Consumer: failed to open channel";
    amqp_destroy_connection(connection_);
    return;
  }

  auto exchange_bytes = amqp_cstring_bytes(exchange_name_.c_str());
  auto queue_bytes = amqp_cstring_bytes(queue_name_.c_str());
  auto type_bytes = amqp_cstring_bytes("topic");

  amqp_exchange_declare(connection_, channel_, exchange_bytes, type_bytes, 0, 1, 0, 0, amqp_empty_table);
  if (amqp_get_rpc_reply(connection_).reply_type != AMQP_RESPONSE_NORMAL) {
    LOG_ERROR() << "Consumer: failed to declare exchange";
    amqp_destroy_connection(connection_);
    return;
  }

  amqp_queue_declare(connection_, channel_, queue_bytes, 0, 1, 0, 0, amqp_empty_table);
  if (amqp_get_rpc_reply(connection_).reply_type != AMQP_RESPONSE_NORMAL) {
    LOG_ERROR() << "Consumer: failed to declare queue";
    amqp_destroy_connection(connection_);
    return;
  }

  const char* bindings[] = {"user.created", "recipe.created", "recipe.ingredient.added", "recipe.favorited"};
  for (const auto* key : bindings) {
    amqp_queue_bind(
        connection_, channel_, queue_bytes,
        exchange_bytes, amqp_cstring_bytes(key),
        amqp_empty_table
    );
  }
  if (amqp_get_rpc_reply(connection_).reply_type != AMQP_RESPONSE_NORMAL) {
    LOG_ERROR() << "Consumer: failed to bind queue";
    amqp_destroy_connection(connection_);
    return;
  }

  amqp_basic_consume(connection_, channel_, queue_bytes, amqp_empty_bytes, 0, 0, 0, amqp_empty_table);
  if (amqp_get_rpc_reply(connection_).reply_type != AMQP_RESPONSE_NORMAL) {
    LOG_ERROR() << "Consumer: failed to start consuming";
    amqp_destroy_connection(connection_);
    return;
  }

  connected_ = true;
  LOG_INFO() << "EventConsumer connected, consuming from queue: " << queue_name_;
}

void EventConsumer::Disconnect() {
  if (connected_) {
    amqp_channel_close(connection_, channel_, AMQP_REPLY_SUCCESS);
    amqp_connection_close(connection_, AMQP_REPLY_SUCCESS);
    amqp_destroy_connection(connection_);
    connected_ = false;
  }
}

void EventConsumer::ConsumeLoop() {
  while (running_) {
    if (!connected_) {
      LOG_WARNING() << "Consumer not connected, retrying in 5s...";
      std::this_thread::sleep_for(std::chrono::seconds(5));
      Connect();
      continue;
    }

    amqp_envelope_t envelope;
    auto res = amqp_consume_message(connection_, &envelope, nullptr, 5000);

    if (res.reply_type == AMQP_RESPONSE_NORMAL) {
      std::string routing_key(
          reinterpret_cast<char*>(envelope.routing_key.bytes),
          envelope.routing_key.len
      );
      std::string body(
          reinterpret_cast<char*>(envelope.message.body.bytes),
          envelope.message.body.len
      );

      ProcessEvent(routing_key, body);

      amqp_destroy_envelope(&envelope);
    } else if (res.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION &&
               res.library_error == AMQP_STATUS_TIMEOUT) {
      continue;
    } else {
      LOG_ERROR() << "Consumer: error receiving message, reconnecting...";
      Disconnect();
    }
  }
}

void EventConsumer::ProcessEvent(const std::string& routing_key, const std::string& body) {
  try {
    auto json = formats::json::FromString(body);
    auto payload = json["payload"];

    if (routing_key == "user.created") {
      HandleUserCreated(payload);
    } else if (routing_key == "recipe.created") {
      HandleRecipeCreated(payload);
    } else if (routing_key == "recipe.ingredient.added") {
      HandleIngredientAdded(payload);
    } else if (routing_key == "recipe.favorited") {
      HandleRecipeFavorited(payload);
    } else {
      LOG_WARNING() << "Unknown routing key: " << routing_key;
    }
  } catch (const std::exception& ex) {
    LOG_ERROR() << "Failed to process event: " << ex.what();
  }
}

void EventConsumer::HandleUserCreated(const formats::json::Value& payload) {
  LOG_INFO() << "Consumer: handling UserCreated event";
  read_model_->OnUserCreated(
      payload["user_id"].As<std::string>(),
      payload["username"].As<std::string>()
  );
}

void EventConsumer::HandleRecipeCreated(const formats::json::Value& payload) {
  LOG_INFO() << "Consumer: handling RecipeCreated event";
  read_model_->OnRecipeCreated(
      payload["recipe_id"].As<std::string>(),
      payload["user_id"].As<std::string>(),
      payload["title"].As<std::string>(),
      payload["cooking_time_minutes"].As<int>()
  );
}

void EventConsumer::HandleIngredientAdded(const formats::json::Value& payload) {
  LOG_INFO() << "Consumer: handling IngredientAdded event";
  read_model_->OnIngredientAdded(
      payload["recipe_id"].As<std::string>(),
      payload["ingredient_id"].As<std::string>(),
      payload["name"].As<std::string>(),
      payload["amount"].As<std::string>(),
      payload["unit"].As<std::string>()
  );
}

void EventConsumer::HandleRecipeFavorited(const formats::json::Value& payload) {
  LOG_INFO() << "Consumer: handling RecipeFavorited event";
  read_model_->OnRecipeFavorited(
      payload["user_id"].As<std::string>(),
      payload["recipe_id"].As<std::string>()
  );
}

}  // namespace recipe_manager

USERVER_NAMESPACE_END
