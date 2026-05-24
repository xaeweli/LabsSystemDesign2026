-- Тестовые данные: Auth Database
\c auth_db

INSERT INTO user_credentials (id, login, password_hash, email, facebook_id, google_id) VALUES
    ('a0000000-0000-0000-0000-000000000001', 'chef_ivan',    'hash_pass_1',  'ivan@example.com',     NULL, 'google_ivan'),
    ('a0000000-0000-0000-0000-000000000002', 'cook_maria',   'hash_pass_2',  'maria@example.com',    NULL, NULL),
    ('a0000000-0000-0000-0000-000000000003', 'baker_pavel',  'hash_pass_3',  'pavel@example.com',    'fb_pavel', NULL),
    ('a0000000-0000-0000-0000-000000000004', 'foodie_anna',  'hash_pass_4',  'anna@example.com',     NULL, NULL),
    ('a0000000-0000-0000-0000-000000000005', 'grill_master', 'hash_pass_5',  'dmitry@example.com',   NULL, NULL),
    ('a0000000-0000-0000-0000-000000000006', 'vegan_olga',   'hash_pass_6',  'olga@example.com',     NULL, 'google_olga'),
    ('a0000000-0000-0000-0000-000000000007', 'sushi_ken',    'hash_pass_7',  'ken@example.com',      NULL, NULL),
    ('a0000000-0000-0000-0000-000000000008', 'pasta_lover',  'hash_pass_8',  'elena@example.com',    NULL, NULL),
    ('a0000000-0000-0000-0000-000000000009', 'spice_girl',   'hash_pass_9',  'natalia@example.com',  'fb_natalia', NULL),
    ('a0000000-0000-0000-0000-00000000000a', 'bbq_king',     'hash_pass_10', 'alexey@example.com',   NULL, NULL);
