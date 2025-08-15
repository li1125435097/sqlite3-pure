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
  result = await sqlite3.exec(`
    CREATE TABLE test (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    int_val INTEGER,        -- 整数类型
    real_val REAL,          -- 浮点数
    text_val TEXT,          -- 文本
    blob_val BLOB,          -- 二进制
    null_val NULL,          -- 空值
    bool_val BOOLEAN,       -- 布尔（SQLite内部存储为整数）
    date_val DATE,          -- 日期（存储为TEXT/INTEGER）
    time_val TIME,          -- 时间（存储为TEXT/INTEGER）
    datetime_val DATETIME   -- 日期时间（存储为TEXT/INTEGER）
  );
    INSERT INTO test VALUES
        (1, 42, 3.14159, 'Hello SQLite', X'010203', NULL, 1, '2025-08-15', '12:34:56', '2025-08-15 12:34:56'),
        (2, -99, 2.71828, '你好世界', X'FFEE', NULL, 0, '2024-01-01', '00:00:00', '2024-01-01 00:00:00')
    ;select * from test;`)
  asserts(result.length === 2, '查询结果数量不正确');
  asserts(result[0].int_val === 42, '整数类型不正确');
  asserts(result[0].real_val === 3.14159, '浮点数类型不正确');
  asserts(result[0].text_val === 'Hello SQLite', '文本类型不正确');
  asserts(result[0].blob_val instanceof Buffer, '二进制类型不正确');
  asserts(result[0].null_val === null, '空值类型不正确');
  asserts(result[0].bool_val === 1, '布尔类型不正确');
  asserts(result[0].date_val === '2025-08-15', '日期类型不正确');
  asserts(result[0].time_val === '12:34:56', '时间类型不正确');
  asserts(result[0].datetime_val === '2025-08-15 12:34:56', '日期时间类型不正确');
  
  
  // 关闭数据库
  sqlite3.closeDb();

  console.log('\x1b[32m%s\x1b[0m', __filename.replace(process.cwd(), '') + ' PASS');
}

module.exports = test;