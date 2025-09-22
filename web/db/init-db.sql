create table sensors (
    mac varchar(17) primary key,
    name varchar(50) default null,
    has_photo boolean default false
);

create table readings (
    mac varchar(17) references sensors(mac) on delete cascade,
    timestamp timestamptz not null,
    humidity float default null,
    temperature float default null,
    battery float default null,
    primary key (mac, timestamp)
)
WITH (
  timescaledb.hypertable,
  timescaledb.partition_column='timestamp',
  timescaledb.segmentby='mac',
  timescaledb.chunk_interval='7d'
);

create table settings (
  key text primary key,
  value text not null
);

insert into settings (key, value) values
  ('sync-time', '12:00'),
  ('battery-warning-threshold', '3.3'),
  ('battery-critical-threshold', '3.0'),
  ('max-latency', '86400'); -- 1 day in seconds

CALL add_columnstore_policy('readings', after => INTERVAL '7d');