workspace "Recipes site" "Домашняя работа 1."{

    model {
        user = person "Пользователь" "Использует сайт для поиска новых рецептов, по названию и ингридиентам, имеет возможность добавлять новые рецепты и сохранять существующие в избранное "
        
        emailSystem = softwareSystem "Email Service" {
            description "Внешний сервис отправки email (авторизация)"
            tags "ExternalSystem"
        }
        facebookauthSystem = softwareSystem "Facebook auth Service" {
            description "Внешний сервис аутентификации через Facebook"
            tags "ExternalSystem"
        }
        googleauthSystem = softwareSystem "Google auth Service" {
            description "Внешний сервис аутентификации через Google"
            tags "ExternalSystem"
        }

        recipesSystem = softwareSystem "Recipes manager System" "Система поиска, добавления, сохранение рецептов, создание коллекций рецептов" {

            webApp = container "Mobile/Web app" "Клиентское приложение пользователя." "React / Flutter"
            apiGateway = container "API Gateway" "Единая точка входа для клиентских запросов." "Spring Boot REST API"

            userauthservice = container "UserAuthService" "Создание нового пользователя, вход в аккаунт, восстановление пароля" "Spring Boot"
            userprofileservice = container "UserProfileService" "Запрос на имя пользователя, аватар, добавленные рецепты пользователя, сохраненные рецепты пользователя (проверка запроса информации о своем профиле или чужом, проверка параметров приватности аккаунта)" "Spring Boot"
            recipepageservice = container "RecipePageService" "Запрос данных о рецепте - название, описание, ингридиенты, автор, добавление рецепта в избранное" "Spring Boot"
            createneditrecipeservice = container "CreateAndEditRecipeService" "Добавление нового рецепта, редактирование существующего (для автора), добавление игридиентов к существующему рецепту" "Spring Boot"
            searchservice = container "SearchService" "Поиск пользователей и рецептов, оформление в список" "Spring Boot"

            cache = container "Cache" "Кэширует часто запрашиваемые данные" "Redis"


            recipesdb = container "Recipes Database" "Хранит данные о рецептах (id, имя, ингриденты, автор, кол-во добавленных в избранное)" "PostgreSQL" "Database"
            usersdb = container "UsersDatabase" "Хранит данные о пользователях (id, username, name, surname, сохраненные рецепты(список ids), добавленные рецепты(список ids))" "PostgreSQL" "Database"
            userauthdb = container "UserAuthDatabase"  "Хранит данные пользователя для аутентификации (id, логин, пароль, почта, фейсбук, гугл аккаунт)"  "PostgreSQL" "Database" 
            
        }

        user -> webApp "Работает с системой" "HTTPS"
        webApp -> apiGateway "Вызывает API" "HTTPS/JSON"
        apiGateway -> userauthservice "Запросы на аутентификацию, регистрацию, смену пароля" "REST/JSON"
        userauthservice -> userauthdb "Запрос к базе данных с логинами и паролями" "JDBC"
        userauthservice -> emailSystem "Отправляет email" "HTTPS"
        userauthservice -> facebookauthSystem "Перенаправляет на страницу входа через Facebook" "HTTPS"
        userauthservice -> googleauthSystem "Перенаправляет на страницу входа через Google" "HTTPS"



        apiGateway -> searchservice "Запросы поиска" "REST/JSON"
        searchservice -> usersdb "Читает информацию из базы данных пользователей, формирует список пользователей согласно запросу" "JDBC"
        searchservice -> recipesdb "Читает информацию из базы данных рецептов, формирует список рецептов согласно запросу" "JDBC"

    
        recipepageservice -> recipesdb "Запрашивает данные о конкретном рецепте" "JDBC"
        recipepageservice -> recipesdb "Повышает счетчик добавленно в избранное для рецепта" "JDBC"
        recipepageservice -> usersdb "Запрашивает данные о пользователе: юзернейм, аватар и т.д." "JDBC"
        recipepageservice -> usersdb "Записывает в базу данных новый рецепт в избранном" "JDBC"



        apiGateway -> userprofileservice "Запрос информации о странице пользователя" "REST/JSON"
        userprofileservice -> usersdb "Получает информацию о пользователе из базы данных пользователей" "JDBC"
        userprofileservice -> recipesdb "Получает информацию о рецептах пользователя из базы данных рецептов" "JDBC"
        

        apiGateway -> createneditrecipeservice "Передает запрос на добавление или изменение рецепта, получает данные от пользователя (название, ингридиенты)" "REST/JSON"
        createneditrecipeservice -> cache "Кэширует данные о рецепте во время создания рецепта" "Redis protocol"
        createneditrecipeservice -> recipesdb "Добавляет новый рецепт в баззу данных рецептов" "JDBC"
        createneditrecipeservice -> usersdb "Добавляет новый рецепт в базу данных пользователей (в список рецептов созданных пользователем)" "JDBC"


        userprofileservice -> cache "Кэширует данные профиля" "Redis protocol"
        recipepageservice -> cache "Кэширует данные рецепта" "Redis protocol"
        searchservice -> cache "Кэширует результаты поиска" "Redis protocol"

    }
    
    views {
        themes default

        systemContext recipesSystem "SystemContext" {
            include user
            include recipesSystem
            include emailSystem
            include facebookauthSystem
            include googleauthSystem

            autoLayout 
        }

        container recipesSystem "Containers" {
            include user
            include *

            autolayout 
        }

        dynamic recipesSystem "AddNewRecipe" {
            title "Добавление нового рецепта"
            autoLayout lr

            user -> webApp "1. Пользователь нажимает на кнопку Добавить рецепт"
            webApp -> apiGateway "2. Запрос на добавление рецепта"
            apiGateway -> createneditrecipeservice "3. Сервис записывает информацию о рецепте данную пользователем"
            createneditrecipeservice ->  cache "4. Информация о рецепте подгружается в кэш"
            createneditrecipeservice -> usersdb "5. Сервис добавляет id рецепта в список добавленных пользователем"
            createneditrecipeservice -> recipesdb "6. Сервис добавляет информацию о рецепте в базу данных рецептов"


        }

        styles {
            element "Person" {
                color #ffffff
                fontSize 22
                shape Person
                width 500
            }
            element "ExternalSystem" {
                background #c0c0c0
                color #ffffff
            }
            element "Container" {
                background #438dd5
                color #ffffff
            }
            element "Database" {
                shape Cylinder
            }

            
        }
    }
}