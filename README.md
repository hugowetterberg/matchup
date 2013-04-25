# Matchup

The matchup module was born out of a need to quickly match users with candidates (in an election) based on a questionnaire.

Matchup operates entirely on buffers, which it hands off to an extension written in C++ to do the actual matching (falling back to a JavaScript implementation if compilation failed). The candidates buffer should contain the following data:

    uint16LE:num_candidates, uint16LE:num_answers
    [uint32LE:candidate_id, [uint8:answer]*num_answers]*num_candidates

So that's a 4-byte header containing the number of candidates in the buffer, followed by the number of answers per candidate as 16bit integers. That's then followed by the candidates which consists of a 32bit integer identifying the candidate plus a series of 8bit integers for the answers.

The result from matchup is also a buffer containing up to ten matches:

    [uint32LE:candidate_id, uint32LE:score]{0-10}

The score will be the sum of the difference between the candidate and the supplied answers, so, lower is better.

## Example

The included example basically does the following. Create sample buffer containing eight candidates with 20 answers each:

    0800 1400
    0000 0000 4b32 3219 6400 3232 0000 4b64 0000 0032 6432 644b
    0100 0000 4b64 3219 4b00 0032 4b4b 004b 3264 6432 6400 6464
    0200 0000 6432 6419 4B4B 0064 0064 1964 3219 6400 6464 6419
    0300 0000 1900 324B 6464 3232 6400 1964 3219 6432 3200 194B
    0400 0000 1900 324B 6464 3232 6400 4b64 0000 0032 6432 644b
    0500 0000 6432 6419 4B4B 0064 0064 004b 3264 6432 6400 6464
    0600 0000 4b64 3219 4b00 0032 4b4b 004b 3264 6432 6464 6419
    0700 0000 4b32 3219 4b00 0032 4b4b 004b 3264 6432 6400 6464

A sample buffer with answers:

    4b64 3219 4b00 0032 4b4b 004b 3264 6432 6400 6464

And then calls `matchup.evaluate` and decodes the result:

    lib.matchup.evaluate(candidates, answers, function(error, data) {
      var i;
      for (i=0; i<data.length; i += 8) {
        console.log(data.readUInt32LE(i), data.readUInt32LE(i+4));
      }
    });

Which is the following:

    1 0
    7 50
    6 175
    5 350
    2 700
    0 700
    3 750
    4 900

## Performance considerations

As a rule of thumb matchup becomes more efficient the bigger the candidate & answer set is. There is also a trade-off when choosing between `matchup.evaluate` and `matchup.evaluateSync`. `evaluateSync` performs the match calculations on the V8 main thread while `evaluate` uses a thread pool.

This means that `evaluate` can utilize multiple CPU cores and frees up the main thread other tasks. But if the candidates that we need to check against are very few, somewhere under a hundred, we'll start seeing diminishing returns when it comes to raw throughput as the thread pool overhead starts approaching the processing time required for evaluating the answer. 
