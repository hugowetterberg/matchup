try {
  module.exports = require('./build/Release/matchup');
}
catch(e) {
  console.error("Failed to load compiled extension, falling back to native implementation.");
  module.exports = require('./matchup.js');
}
