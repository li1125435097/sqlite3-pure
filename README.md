# sqlite3-pure

A simple Node.js addon for SQLite3 with embedded SQLite3 source (version 3.50.4), eliminating external dependencies.

## Installation

```bash
npm install sqlite3-pure
```

## Usage

```javascript
const sqlite3 = require('sqlite3-pure');

// Open a database， Global singleton, only supports one database
sqlite3.openDb('test.db');

// Execute SQL statements， Supports multiple statements separated by semicolons, Support transactions, Returns a promise
sqlite3.exec('CREATE TABLE if not exists test (id INTEGER, name TEXT); INSERT INTO test VALUES (1, "Alice"), (2, "Bob"); SELECT * FROM test;').then(async a => {
    // Fetch results
    let result = await sqlite3.exec('SELECT * FROM test')
    console.log(result);
    
    // Close the database
    sqlite3.closeDb();
})

```

## Prerequisites

- Node.js >= 14
- No external SQLite3 library required (embedded in the module)

## Version Notes
- 0.0.5-beta: Fix the issue of converting JS data types to SQLite data types

## License

MIT