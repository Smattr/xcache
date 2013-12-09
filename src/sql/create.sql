create table if not exists operation (
    id integer primary key autoincrement,
    cwd text not null,
    command text not null);

create table if not exists input (
    fk_operation integer references operation(id),
    filename text not null,
    timestamp integer not null);

create table if not exists output (
    fk_operation integer references operation(id),
    filename text not null,
    timestamp integer not null,
    contents text not null);
