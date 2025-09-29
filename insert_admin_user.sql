-- Insert admin user with no limits
-- Username: paramorphix
-- Password: hacket@1#12
-- Admin rank: 1 (admin)
-- No cooldowns: 0
-- No limits: 0

INSERT INTO users (username, password, max_bots, admin, last_paid, cooldown, duration_limit) 
VALUES ('paramorphix', 'hacket@1#12', 0, 1, UNIX_TIMESTAMP(), 0, 0);

-- Verify the user was created
SELECT username, admin, max_bots, cooldown, duration_limit FROM users WHERE username = 'paramorphix';
