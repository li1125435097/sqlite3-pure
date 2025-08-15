const sqlite3 = require('../index');
const { rmSync } = require('fs')
const asserts = require('assert');

// 数据库选择
const databases = ['test.db', ':memory:']; // 使用实际数据库 和内存数据库
const db = databases[1];  // 使用实际数据库


async function test() {
  // 打开数据库
  let result = sqlite3.openDb(db);
  asserts(!result, '打开数据库失败');

  // 创建表并插入数据
  result = await sqlite3.exec('CREATE TABLE test (id INTEGER, name TEXT); INSERT INTO test VALUES (1, "Alice"), (2, "Bob"); SELECT * FROM test;')
  asserts(result.length === 2, '查询结果数量不正确');
  console.log(result);
  
  
  // 关闭数据库
  sqlite3.closeDb();
  

  console.log('\x1b[32m%s\x1b[0m', __filename.replace(process.cwd(), '') + ' PASS');
}

module.exports = test;