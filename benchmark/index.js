var lib = {
  fs: require('fs'),
  t2apps: require('t2apps'),
  async: require('async'),
  matchup: require('../build/Release/matchup'),
  matchupjs: require('../matchup.js')
};

var test = function(matchup, sampleSize, iterations, callback) {
  var segments = {}, answer;
  answer = new Buffer(20);
  answer.fill(50);

  segments.a = lib.fs.readFileSync('./data/a.bin');

  var buffer = matchup.evaluateSync(segments.a, answer), offset = 0;
  process.stdout.write("Result: ");
  while (offset < buffer.length) {
    process.stdout.write(buffer.readUInt32LE(offset) + ":" + buffer.readUInt32LE(offset+4) + " ");
    offset += 8;
  }
  process.stdout.write("\n");

  var i, t, time, delta;
  for (t=0; t<iterations; t++) {
    time = new Date();
    for (i=0; i<sampleSize; i++) {
      matchup.evaluateSync(segments.a, answer);
    }
    delta = new Date().getTime() - time.getTime();
    console.log("Sync Time", delta);
    console.log("Sync answers/ms", Math.round(sampleSize/delta*100)/100);
  }

  var counter = 0;
  var batchCounter = 0;
  var workDone = function() {
    delta = new Date().getTime() - time.getTime();
    console.log("Async Time", delta);
    console.log("Async answers/ms", Math.round(sampleSize/delta*100)/100);

    if (batchCounter < iterations) {
      startBatch();
    }
    else {
      callback();
    }
  };
  var doWork = function() {
    var itemNum = ++counter;
    matchup.evaluate(segments.a, answer, function(error, data) {
      if (counter < sampleSize) {
        doWork();
      }
      if (itemNum == sampleSize) {
        workDone();
      }
    });
  };
  var startBatch = function() {
    batchCounter++;
    counter = 0;
    time = new Date();
    for (var c=0; c<8; c++) {
      doWork()
    }
  };

  startBatch();
};

console.error("C++ implementation");
test(lib.matchup, 100000, 1, function() {
  console.error("\nJavascript implementation");
  test(lib.matchupjs, 100000, 1, function() {
    console.error("\nDone");
  });
});
