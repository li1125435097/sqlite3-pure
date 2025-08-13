const addon = require('bindings')('sqlite3-simple');

module.exports = {
  openDb: addon.OpenDb,
  exec: addon.Exec
};