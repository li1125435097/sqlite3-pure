# sqlite3-pure

A simple Node.js addon for SQLite3 with embedded SQLite3 source (version 3.50.4), eliminating external dependencies.

## Installation

```bash
npm install sqlite3-pure
```

## Usage

```javascript
const sqlite3 = require('sqlite3-pure');

sqlite3.openDb(':memory:'); // Use in-memory database
const results = sqlite3.exec('CREATE TABLE test (id INTEGER, name TEXT); INSERT INTO test VALUES (1, "Alice"), (2, "Bob"); SELECT * FROM test;');
console.log(results);
// Output: [{ id: '1', name: 'Alice' }, { id: '2', name: 'Bob' }]
```

## Prerequisites

- Node.js >= 14
- No external SQLite3 library required (embedded in the module)

## Development

To build locally:

```bash
npm install
npm run build
npm test
```

## License

MIT