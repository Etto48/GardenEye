create table sensors (
    mac varchar(17) primary key,
    name varchar(50) default null
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

CALL add_columnstore_policy('readings', after => INTERVAL '7d');