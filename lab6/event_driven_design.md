# Event-Driven Architecture Design

## Variant 23 — Recipe Management System (Allrecipes)

---

## 1. Domain Analysis

### Bounded Contexts

| Context | Description | Entities |
|---------|-------------|----------|
| **User Management** | Registration, authentication, profile management | User, Credentials |
| **Recipe Management** | CRUD for recipes, ingredients management | Recipe, Ingredient |
| **Favorites** | Managing user's favorite recipes | Favorites |
| **Search** | Searching users and recipes | - |

### Domain Events

| Event | Description | Producer Context | Consumer Context |
|-------|-------------|------------------|------------------|
| `UserCreated` | New user registered | User Management | Recipe Management, Favorites, Search |
| `RecipeCreated` | New recipe published | Recipe Management | Search, User Management |
| `IngredientAdded` | Ingredient added to recipe | Recipe Management | Search |
| `RecipeFavorited` | Recipe added to user favorites | Favorites | Recipe Management (favorite count) |
| `RecipeSearched` | User searched for recipes | Search | Analytics (hypothetical) |

### Commands

| Command | Description | Triggered By |
|---------|-------------|-------------|
| `CreateUser` | Register a new user | `POST /users` |
| `CreateRecipe` | Create a new recipe | `POST /recipes` |
| `AddIngredient` | Add ingredient to recipe | `POST /recipes/{id}/ingredients` |
| `AddFavorite` | Add recipe to favorites | `POST /users/{id}/favorites` |

---

## 2. Event-Driven Architecture

### Mapping to Lab1 Container Architecture

| Lab1 Container | Role in Lab6 | Event Producer? | Event Consumer? |
|----------------|-------------|-----------------|-----------------|
| **API Gateway** | Unified entry point (`/` root handler) | — | — |
| **UserAuth Service** | `CreateUserHandler`, `FindUserByLoginHandler` | `UserCreated` | — |
| **Userprofile Service** | `SearchUsersByNameHandler` | — | — |
| **Create and Edit Recipe Service** | `CreateRecipeHandler`, `AddIngredientHandler` | `RecipeCreated`, `IngredientAdded` | — |
| **Recipepage Service** | `GetRecipesHandler`, `GetIngredientsHandler`, `GetFavoritesHandler` | — | — |
| **Search Service** | `SearchRecipesHandler` | — | — |
| **Recipes Database** | `RecipeStorage` (write model) | — | — |
| **Users Database** | `RecipeStorage` (user profiles) | — | — |
| **Redis Cache** | Replaced by RabbitMQ event bus | — | — |

### Architecture Overview

```
                         ┌──────────────────────┐
                         │    HTTP Clients       │
                         └──────────┬───────────┘
                                    │
                                    ▼
                         ┌──────────────────────┐
                         │    API Gateway (/)    │
                         └──────────┬───────────┘
                                    │
             ┌──────────────────────┼──────────────────────┐
             │                      │                      │
             ▼                      ▼                      ▼
   ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐
   │  User Handlers   │  │  Recipe Handlers │  │ Favorites        │
   │  POST/GET /users │  │  POST/GET /recipes│  │ POST/GET fav.    │
   └────────┬─────────┘  └────────┬─────────┘  └────────┬─────────┘
            │                     │                      │
            └─────────────────────┼──────────────────────┘
                                  │
                                  ▼
                    ┌─────────────────────────┐
                    │   RabbitMQ Exchange     │
                    │   "recipe.events"       │
                    └─────────────┬───────────┘
                                  │
                    ┌─────────────┼─────────────┐
                    │             │             │
                    ▼             ▼             ▼
          ┌──────────────┐ ┌──────────┐ ┌──────────┐
          │ Read Model   │ │ Search   │ │ Analytics│
          │ Updater      │ │ Index    │ │ Logger   │
          │ (consumed)   │ │ (planned)│ │ (planned)│
          └──────┬───────┘ └──────────┘ └──────────┘
                 │
                 ▼
          ┌──────────────┐
          │ RecipeRead   │
          │ Model        │
          │ (CQRS query) │
          └──────────────┘
```

### Event Producers

| Producer | Events Published | Exchange | Routing Key |
|----------|-----------------|----------|-------------|
| User Service | `UserCreated` | `recipe.events` | `user.created` |
| Recipe Service | `RecipeCreated`, `IngredientAdded` | `recipe.events` | `recipe.created`, `recipe.ingredient.added` |
| Favorites Service | `RecipeFavorited` | `recipe.events` | `recipe.favorited` |

### Event Consumers

| Consumer | Events Subscribed | Queue | Action |
|----------|------------------|-------|--------|
| Read Model Updater | `UserCreated`, `RecipeCreated`, `IngredientAdded`, `RecipeFavorited` | `recipe.read-model` | Update CQRS read model projection |
| Search Index Updater | `RecipeCreated`, `IngredientAdded` | `recipe.search-index` | Update search index |
| Analytics Logger | `UserCreated`, `RecipeCreated`, `RecipeFavorited`, `RecipeSearched` | `recipe.analytics` | Log events for analytics |

### Complete Event Flow Diagrams

#### Flow 1: User Registration
```
Client                  Handler                Write Model            RabbitMQ              Read Model
  │                        │                       │                     │                     │
  │  POST /users           │                       │                     │                     │
  │───────────────────────>│                       │                     │                     │
  │                        │  CreateUser()         │                     │                     │
  │                        │──────────────────────>│                     │                     │
  │                        │     User               │                     │                     │
  │                        │<──────────────────────│                     │                     │
  │                        │                       │                     │                     │
  │                        │  Publish(UserCreated)  │                     │                     │
  │                        │────────────────────────────────────────────>│                     │
  │                        │                       │                     │  Consume             │
  │                        │                       │                     │─────────────────────>│
  │                        │                       │                     │                     │
  │                        │                       │                     │  OnUserCreated()     │
  │                        │                       │                     │                     │
  │  201 Created           │                       │                     │                     │
  │<───────────────────────│                       │                     │                     │
```

#### Flow 2: Recipe Creation
```
Client                  Handler                Write Model            RabbitMQ              Read Model
  │                        │                       │                     │                     │
  │  POST /recipes         │                       │                     │                     │
  │───────────────────────>│                       │                     │                     │
  │                        │  CreateRecipe()       │                     │                     │
  │                        │──────────────────────>│                     │                     │
  │                        │     Recipe             │                     │                     │
  │                        │<──────────────────────│                     │                     │
  │                        │                       │                     │                     │
  │                        │  Publish(RecipeCreated)│                     │                     │
  │                        │────────────────────────────────────────────>│                     │
  │                        │                       │                     │  Consume             │
  │                        │                       │                     │─────────────────────>│
  │                        │                       │                     │                     │
  │                        │                       │                     │  OnRecipeCreated()   │
  │                        │                       │                     │                     │
  │  201 Created           │                       │                     │                     │
  │<───────────────────────│                       │                     │                     │
```

#### Flow 3: Adding Ingredient
```
Client                  Handler                Write Model            RabbitMQ              Read Model
  │                        │                       │                     │                     │
  │  POST /recipes/{id}/   │                       │                     │                     │
  │  ingredients           │                       │                     │                     │
  │───────────────────────>│                       │                     │                     │
  │                        │  AddIngredient()      │                     │                     │
  │                        │──────────────────────>│                     │                     │
  │                        │   Ingredient          │                     │                     │
  │                        │<──────────────────────│                     │                     │
  │                        │                       │                     │                     │
  │                        │  Publish(Ingredient   │                     │                     │
  │                        │  Added)               │                     │                     │
  │                        │────────────────────────────────────────────>│                     │
  │                        │                       │                     │  Consume             │
  │                        │                       │                     │─────────────────────>│
  │                        │                       │                     │                     │
  │                        │                       │                     │  OnIngredientAdded() │
  │                        │                       │                     │                     │
  │  201 Created           │                       │                     │                     │
  │<───────────────────────│                       │                     │                     │
```

#### Flow 4: Read (CQRS Query)
```
Client                  Handler                Read Model (CQRS)
  │                        │                       │
  │  GET /recipes          │                       │
  │───────────────────────>│                       │
  │                        │  GetAllRecipes()      │
  │                        │──────────────────────>│
  │                        │  [RecipeSummary...]   │
  │                        │<──────────────────────│
  │  JSON array            │                       │
  │<───────────────────────│                       │
```

---

## 3. Messaging System: RabbitMQ

### Why RabbitMQ?

| Criteria | RabbitMQ | Apache Kafka |
|----------|----------|--------------|
| Use case | Event-driven services | Event streaming / data pipelines |
| Routing | Flexible (topic, direct, fanout) | Topic-based |
| Message model | Push-based | Pull-based |
| Complexity | Lower | Higher |
| Ordering | Per-queue | Per-partition |
| Retention | Ack-based deletion | Configurable retention |

**Chosen: RabbitMQ** — better fit for service-to-service event communication with flexible routing.

### Exchange and Queue Configuration

| Exchange | Type | Bindings |
|----------|------|----------|
| `recipe.events` | `topic` | All event routes |

| Queue | Routing Keys | Bind To |
|-------|-------------|---------|
| `recipe.read-model` | `user.created`, `recipe.created`, `recipe.ingredient.added`, `recipe.favorited` | `recipe.events` |
| `recipe.search-index` | `recipe.created`, `recipe.ingredient.added` | `recipe.events` |
| `recipe.analytics` | `user.created`, `recipe.created`, `recipe.favorited` | `recipe.events` |

### Delivery Guarantees

| Property | Value | Implementation |
|----------|-------|----------------|
| Delivery mode | **At-least-once** | Consumer acks after processing; publisher confirms |
| Queue durability | **Durable** | Queues survive broker restart |
| Message persistence | **Persistent** | Messages survive broker restart |
| Consumer ACK | **Manual** | Ack only after successful processing |

---

## 4. CQRS Pattern

### Applicability

CQRS is **partially applicable** to this system:

| Aspect | Decision | Reasoning |
|--------|----------|-----------|
| Read vs Write separation | Yes | Read model can be optimized for queries |
| Separate databases | No | Both models in-memory for simplicity |
| Event-sourced write model | No | Write model is state-based, not event-sourced |

### Read Model (Projection)

The read model maintains denormalized views updated via events:

| Projection | Updated By | Purpose |
|------------|-----------|---------|
| `RecipeSummary` | `RecipeCreated`, `IngredientAdded` | Lightweight recipe list for `GET /recipes` |
| `UserProfileView` | `UserCreated` | User profile for search results |
| `FavoriteCount` | `RecipeFavorited` | Aggregated favorite counts |

### Synchronization

```
Write (Command) Side                Read (Query) Side
┌─────────────────┐                ┌─────────────────┐
│ RecipeStorage   │                │ RecipeReadModel │
│ (In-memory)     │                │ (In-memory)     │
└────────┬────────┘                └────────▲────────┘
         │                                  │
         │  Publish event                   │  Consume event
         ▼                                  │
   ┌──────────┐                             │
   │ RabbitMQ │─────────────────────────────┘
   │  Events  │  Route to read-model queue
   └──────────┘
```

---

## 5. Event Payload Schemas

### UserCreated
```json
{
  "event_type": "UserCreated",
  "event_id": "uuid",
  "timestamp": "2026-05-24T12:00:00Z",
  "payload": {
    "user_id": "uuid",
    "username": "chef23",
    "email": "chef@example.com"
  }
}
```

### RecipeCreated
```json
{
  "event_type": "RecipeCreated",
  "event_id": "uuid",
  "timestamp": "2026-05-24T12:00:00Z",
  "payload": {
    "recipe_id": "uuid",
    "user_id": "uuid",
    "title": "Apple Pie",
    "cooking_time_minutes": 60
  }
}
```

### IngredientAdded
```json
{
  "event_type": "IngredientAdded",
  "event_id": "uuid",
  "timestamp": "2026-05-24T12:00:00Z",
  "payload": {
    "recipe_id": "uuid",
    "ingredient_id": "uuid",
    "name": "Apple",
    "amount": "4",
    "unit": "pcs"
  }
}
```

### RecipeFavorited
```json
{
  "event_type": "RecipeFavorited",
  "event_id": "uuid",
  "timestamp": "2026-05-24T12:00:00Z",
  "payload": {
    "user_id": "uuid",
    "recipe_id": "uuid"
  }
}
```
