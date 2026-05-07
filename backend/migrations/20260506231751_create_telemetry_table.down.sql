-- Drop policies first (dependencies)
DROP POLICY IF EXISTS "Allow all inserts" ON telemetry;
DROP POLICY IF EXISTS "Allow public read-only access" ON telemetry;

-- Drop indexes
DROP INDEX IF EXISTS idx_telemetry_device_time;
DROP INDEX IF EXISTS idx_telemetry_recorded_at;

-- Drop table
DROP TABLE IF EXISTS telemetry;