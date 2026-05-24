#include "handlers.hpp"

#include <optional>
#include <string>

#include <userver/formats/json.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/http/content_type.hpp>
#include <userver/server/http/http_status.hpp>

#include "api_utils.hpp"

USERVER_NAMESPACE_BEGIN

namespace recipe_manager {

namespace {

formats::json::Value RecipesToJson(const RecipeReadModel& model, const std::vector<RecipeSummary>& recipes) {
  formats::json::ValueBuilder builder(formats::common::Type::kArray);
  for (const auto& recipe : recipes) {
    builder.PushBack(model.RecipeSummaryToJson(recipe));
  }
  return builder.ExtractValue();
}

}  // namespace

// ========== BaseHandler ==========

BaseHandler::BaseHandler(const components::ComponentConfig& config, const components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      storage_(context.FindComponent<RecipeStorage>()),
      producer_(context.FindComponent<EventProducer>()) {}

// ========== RootHandler ==========

std::string RootHandler::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext&
) const {
  formats::json::ValueBuilder endpoints(formats::common::Type::kArray);
  endpoints.PushBack("POST /users");
  endpoints.PushBack("GET /users/login/{username}");
  endpoints.PushBack("GET /users/search?first_name=Iv*&last_name=Pet*");
  endpoints.PushBack("POST /recipes");
  endpoints.PushBack("GET /recipes");
  endpoints.PushBack("GET /recipes/search?title=*pie*");
  endpoints.PushBack("POST /recipes/{recipe_id}/ingredients");
  endpoints.PushBack("GET /recipes/{recipe_id}/ingredients");
  endpoints.PushBack("GET /users/{user_id}/recipes");
  endpoints.PushBack("POST /users/{user_id}/favorites");
  endpoints.PushBack("GET /users/{user_id}/favorites");

  formats::json::ValueBuilder builder(formats::common::Type::kObject);
  builder["service"] = "recipe-manager-event-driven";
  builder["status"] = "ok";
  builder["variant"] = "23";
  builder["description"] = "Recipe management API with Event-Driven architecture, RabbitMQ and CQRS";
  builder["endpoints"] = endpoints.ExtractValue();

  request.GetHttpResponse().SetContentType(http::content_type::kApplicationJson);
  return formats::json::ToString(builder.ExtractValue());
}

// ========== CreateUserHandler ==========

CreateUserHandler::CreateUserHandler(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
) : HttpHandlerJsonBase(config, context),
    storage_(context.FindComponent<RecipeStorage>()),
    producer_(context.FindComponent<EventProducer>()) {}

formats::json::Value CreateUserHandler::HandleRequestJsonThrow(
    const server::http::HttpRequest& request,
    const formats::json::Value& request_json,
    server::request::RequestContext&
) const {
  try {
    const auto user = storage_.CreateUser(
        RequireField<std::string>(request_json, "username"),
        RequireField<std::string>(request_json, "first_name"),
        RequireField<std::string>(request_json, "last_name"),
        RequireField<std::string>(request_json, "email"),
        RequireField<std::string>(request_json, "password")
    );

    producer_.Publish(MakeUserCreatedEvent(
        {}, user.id, user.username, user.email
    ));

    request.GetHttpResponse().SetStatus(server::http::HttpStatus::kCreated);
    return storage_.UserToJson(user);
  } catch (const ApiException& ex) {
    request.GetHttpResponse().SetStatus(ex.GetStatus());
    return ErrorJson(ex.what());
  }
}

// ========== FindUserByLoginHandler ==========

std::string FindUserByLoginHandler::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext&
) const {
  return Execute(request, [&] {
    const auto username = request.GetPathArg("username");
    const auto user = storage_.FindUserByUsername(username);
    if (!user) {
      throw ApiException(server::http::HttpStatus::kNotFound, "User not found");
    }
    return storage_.UserToJson(*user);
  });
}

// ========== SearchUsersByNameHandler ==========

std::string SearchUsersByNameHandler::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext&
) const {
  return Execute(request, [&] {
    formats::json::ValueBuilder builder(formats::common::Type::kArray);
    const auto users = storage_.SearchUsersByNameMask(
        request.HasArg("first_name") ? std::make_optional(request.GetArg("first_name")) : std::nullopt,
        request.HasArg("last_name") ? std::make_optional(request.GetArg("last_name")) : std::nullopt
    );
    for (const auto& user : users) {
      builder.PushBack(storage_.UserToJson(user));
    }
    return builder.ExtractValue();
  });
}

// ========== CreateRecipeHandler ==========

CreateRecipeHandler::CreateRecipeHandler(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
) : HttpHandlerJsonBase(config, context),
    storage_(context.FindComponent<RecipeStorage>()),
    producer_(context.FindComponent<EventProducer>()) {}

formats::json::Value CreateRecipeHandler::HandleRequestJsonThrow(
    const server::http::HttpRequest& request,
    const formats::json::Value& request_json,
    server::request::RequestContext&
) const {
  try {
    const auto recipe = storage_.CreateRecipe(
        RequireField<std::string>(request_json, "user_id"),
        RequireField<std::string>(request_json, "title"),
        OptionalString(request_json, "description").value_or(""),
        RequireField<std::string>(request_json, "instructions"),
        RequireField<int>(request_json, "cooking_time_minutes")
    );

    producer_.Publish(MakeRecipeCreatedEvent(
        {}, recipe.id, recipe.user_id, recipe.title, recipe.cooking_time_minutes
    ));

    request.GetHttpResponse().SetStatus(server::http::HttpStatus::kCreated);
    return storage_.RecipeToJson(recipe);
  } catch (const ApiException& ex) {
    request.GetHttpResponse().SetStatus(ex.GetStatus());
    return ErrorJson(ex.what());
  }
}

// ========== GetRecipesHandler (from read model) ==========

GetRecipesHandler::GetRecipesHandler(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
) : BaseHandler(config, context),
    read_model_(context.FindComponent<RecipeReadModel>()) {}

std::string GetRecipesHandler::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext&
) const {
  return Execute(request, [&] {
    return RecipesToJson(read_model_, read_model_.GetAllRecipes());
  });
}

// ========== SearchRecipesHandler (from read model) ==========

SearchRecipesHandler::SearchRecipesHandler(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
) : BaseHandler(config, context),
    read_model_(context.FindComponent<RecipeReadModel>()) {}

std::string SearchRecipesHandler::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext&
) const {
  return Execute(request, [&] {
    if (!request.HasArg("title")) {
      throw ApiException(server::http::HttpStatus::kBadRequest, "title is required");
    }
    return RecipesToJson(read_model_, read_model_.SearchRecipes(request.GetArg("title")));
  });
}

// ========== AddIngredientHandler ==========

AddIngredientHandler::AddIngredientHandler(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
) : HttpHandlerJsonBase(config, context),
    storage_(context.FindComponent<RecipeStorage>()),
    producer_(context.FindComponent<EventProducer>()) {}

formats::json::Value AddIngredientHandler::HandleRequestJsonThrow(
    const server::http::HttpRequest& request,
    const formats::json::Value& request_json,
    server::request::RequestContext&
) const {
  try {
    const auto ingredient = storage_.AddIngredientToRecipe(
        request.GetPathArg("recipe_id"),
        RequireField<std::string>(request_json, "name"),
        RequireField<std::string>(request_json, "amount"),
        OptionalString(request_json, "unit").value_or("")
    );

    producer_.Publish(MakeIngredientAddedEvent(
        {}, request.GetPathArg("recipe_id"),
        ingredient.id, ingredient.name,
        ingredient.amount, ingredient.unit
    ));

    request.GetHttpResponse().SetStatus(server::http::HttpStatus::kCreated);
    return storage_.IngredientToJson(ingredient);
  } catch (const ApiException& ex) {
    request.GetHttpResponse().SetStatus(ex.GetStatus());
    return ErrorJson(ex.what());
  }
}

// ========== GetIngredientsHandler ==========

std::string GetIngredientsHandler::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext&
) const {
  return Execute(request, [&] {
    formats::json::ValueBuilder builder(formats::common::Type::kArray);
    for (const auto& ingredient : storage_.GetRecipeIngredients(request.GetPathArg("recipe_id"))) {
      builder.PushBack(storage_.IngredientToJson(ingredient));
    }
    return builder.ExtractValue();
  });
}

// ========== GetUserRecipesHandler (from read model) ==========

GetUserRecipesHandler::GetUserRecipesHandler(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
) : BaseHandler(config, context),
    read_model_(context.FindComponent<RecipeReadModel>()) {}

std::string GetUserRecipesHandler::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext&
) const {
  return Execute(request, [&] {
    return RecipesToJson(read_model_, read_model_.GetUserRecipes(request.GetPathArg("user_id")));
  });
}

// ========== AddFavoriteHandler ==========

AddFavoriteHandler::AddFavoriteHandler(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
) : HttpHandlerJsonBase(config, context),
    storage_(context.FindComponent<RecipeStorage>()),
    producer_(context.FindComponent<EventProducer>()) {}

formats::json::Value AddFavoriteHandler::HandleRequestJsonThrow(
    const server::http::HttpRequest& request,
    const formats::json::Value& request_json,
    server::request::RequestContext&
) const {
  try {
    const auto recipe = storage_.AddFavoriteRecipe(
        request.GetPathArg("user_id"),
        RequireField<std::string>(request_json, "recipe_id")
    );

    producer_.Publish(MakeRecipeFavoritedEvent(
        {}, request.GetPathArg("user_id"),
        request_json["recipe_id"].As<std::string>()
    ));

    return storage_.RecipeToJson(recipe);
  } catch (const ApiException& ex) {
    request.GetHttpResponse().SetStatus(ex.GetStatus());
    return ErrorJson(ex.what());
  }
}

// ========== GetFavoritesHandler (from read model) ==========

GetFavoritesHandler::GetFavoritesHandler(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
) : BaseHandler(config, context),
    read_model_(context.FindComponent<RecipeReadModel>()) {}

std::string GetFavoritesHandler::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext&
) const {
  return Execute(request, [&] {
    return RecipesToJson(read_model_, read_model_.GetFavoriteRecipes(request.GetPathArg("user_id")));
  });
}

// ========== Component List ==========

components::ComponentList MakeRecipeComponentList() {
  return components::MinimalServerComponentList()
      .Append<RecipeStorage>()
      .Append<RecipeReadModel>()
      .Append<EventProducer>()
      .Append<EventConsumer>()
      .Append<RootHandler>()
      .Append<CreateUserHandler>()
      .Append<FindUserByLoginHandler>()
      .Append<SearchUsersByNameHandler>()
      .Append<CreateRecipeHandler>()
      .Append<GetRecipesHandler>()
      .Append<SearchRecipesHandler>()
      .Append<AddIngredientHandler>()
      .Append<GetIngredientsHandler>()
      .Append<GetUserRecipesHandler>()
      .Append<AddFavoriteHandler>()
      .Append<GetFavoritesHandler>();
}

}  // namespace recipe_manager

USERVER_NAMESPACE_END
