const addon = require('bindings')('sqlite3-pure');

module.exports = {
  openDb: addon.OpenDb,
  closeDb: addon.CloseDb,
  exec: addon.Exec
};