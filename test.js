const sqlite3 = require('./index');
sqlite3.openDb(':memory:');
sqlite3.exec('CREATE TABLE test (id INTEGER, name TEXT, score REAL, data BLOB, flag NULL); ' +
             'INSERT INTO test VALUES (1, "Alice", 95.5, X"48656C6C6F", NULL); ' +
             'INSERT INTO test VALUES (2, "Bob", 88.0, X"776F726C64", NULL); ' +
             'SELECT * FROM test;').then(results => {
    console.log(results);
});