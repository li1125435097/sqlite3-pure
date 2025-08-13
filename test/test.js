const sqlite3 = require('../index');

sqlite3.openDb(':memory:'); // 使用内存数据库
const results = sqlite3.exec('CREATE TABLE test (id INTEGER, name TEXT); INSERT INTO test VALUES (1, "Alice"), (2, "Bob"); SELECT * FROM test;');
console.log(results); // 预期输出：[{ id: '1', name: 'Alice' }, { id: '2', name: 'Bob' }]