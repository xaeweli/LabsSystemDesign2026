-- Auth Database: учётные данные пользователей
\c auth_db

CREATE TABLE IF NOT EXISTS user_credentials (
    id            UUID PRIMARY KEY,
    login         VARCHAR(50)  NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    email         VARCHAR(255) NOT NULL,
    facebook_id   VARCHAR(255) DEFAULT NULL,
    google_id     VARCHAR(255) DEFAULT NULL,

    CONSTRAINT uq_credentials_login UNIQUE (login),
    CONSTRAINT uq_credentials_email UNIQUE (email),
    CONSTRAINT chk_credentials_login_length CHECK (char_length(login) >= 3 AND char_length(login) <= 50),
    CONSTRAINT chk_credentials_password_length CHECK (char_length(password_hash) >= 6),
    CONSTRAINT chk_credentials_email_format CHECK (email ~* '^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}$')
);

CREATE INDEX IF NOT EXISTS idx_credentials_login ON user_credentials(login);
