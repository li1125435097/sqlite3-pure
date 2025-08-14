const addon = require('bindings')('sqlite3-pure');

module.exports = {
  openDb: addon.OpenDb,
  exec: addon.Exec
};