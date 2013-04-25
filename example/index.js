var lib = {
  matchup: require('../index.js')
};

var
  candidates_hex, candidates,
  answers_hex, answers;

// Set up our small candidate data set
candidates_hex = (
  "0800 1400" +
  "0000 0000 4b32 3219 6400 3232 0000 4b64 0000 0032 6432 644b" +
  "0100 0000 4b64 3219 4b00 0032 4b4b 004b 3264 6432 6400 6464" +
  "0200 0000 6432 6419 4B4B 0064 0064 1964 3219 6400 6464 6419" +
  "0300 0000 1900 324B 6464 3232 6400 1964 3219 6432 3200 194B" +
  "0400 0000 1900 324B 6464 3232 6400 4b64 0000 0032 6432 644b" +
  "0500 0000 6432 6419 4B4B 0064 0064 004b 3264 6432 6400 6464" +
  "0600 0000 4b64 3219 4b00 0032 4b4b 004b 3264 6432 6464 6419" +
  "0700 0000 4b32 3219 4b00 0032 4b4b 004b 3264 6432 6400 6464"
).replace(/\s/g, '');

// Create an answer that
answers_hex = (
  "4b64 3219 4b00 0032 4b4b 004b 3264 6432 6400 6464"
).replace(/\s/g, '');

candidates = new Buffer(candidates_hex, 'hex');
answers = new Buffer(answers_hex, 'hex');

lib.matchup.evaluate(candidates, answers, function(error, data) {
  var i;
  for (i=0; i<data.length; i += 8) {
    console.log(data.readUInt32LE(i), data.readUInt32LE(i+4));
  }
});