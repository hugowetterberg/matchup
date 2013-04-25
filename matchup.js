
var util = {
  processData: function(answers, candidates) {
    var offset = 0, i, j, score,
      candidateCount = candidates.readUInt16LE(offset),
      answerCount = candidates.readUInt16LE(offset + 2),
      first, last, tmp;
    offset += 4;

    for (i = 0; i < candidateCount; i++) {
      score = {
        id: candidates.readUInt32LE(offset),
        score: 0,
        next: null,
        previous: null
      };
      offset += 4;

      for (j=0; j<answerCount; j++) {
        score.score += Math.abs(candidates.readUInt8(offset+j) - answers.readUInt8(j));
        // Abort evaluation if i>10 and score > last->score
        if (i > 10 && score.score > last.score) {
            delete score;
            score = null;
            break;
        }
      }
      offset += answerCount;

      if (!score) {
        continue;
      }

      if (!first) {
        first = last = score;
      }
      else {
        tmp = last;
        // Walk from the end to the beginning to find an item with a lower
        // score that we can add our result after.
        do {
          // If we have a score lower than the current item
          if (score.score <= tmp.score) {
            // We walk up if we're not at the beginning of the list
            if (tmp.previous) {
              tmp = tmp.previous;
            }
            else {
              // Otherwise we add the result to the beginning of the
              // list.
              first = score;
              first.next = tmp;
              tmp.previous = first;
              break;
            }
          }
          else {
            // Don't add new items at the end of the list if we already
            // have 10 items.
            if (i >= 10 && !tmp.next) {
              delete score;
              score = null;
            }
            else {
              // Insert the result after the item with a lower score.
              score.next = tmp.next;
              score.previous = tmp;
              if (score.next) {
                score.next.previous = score;
              }
              tmp.next = score;
              if (!score.next) {
                last = score;
              }
            }
            break;
          }
        } while(tmp);
      }

      // Pop off the last item if the list is getting crowded.
      if (score && (score.next || score.previous) && i >= 10) {
        tmp = last.previous;
        delete last;
        last = tmp;
        last.next = null;
      }
    }
    return first;
  },
  createResultBuffer: function(candidates, result) {
    var candidateCount = candidates.readUInt16LE(0),
      resultCount = Math.min(10, candidateCount),
      buffer = new Buffer(resultCount*8), offset = 0,
      current = result;

    while (current && offset < buffer.length) {
      buffer.writeUInt32LE(current.id, offset); offset += 4;
      buffer.writeUInt32LE(current.score, offset); offset += 4;
      current = current.next;
    }

    return buffer;
  }
};

exports.evaluateSync = function(candidates, answers) {
  var result = util.processData(answers, candidates);
  return util.createResultBuffer(candidates, result);
};

exports.evaluate = function(candidates, answers, callback) {
  var result = util.processData(answers, candidates);
  setImmediate(function() {
    callback(null, util.createResultBuffer(candidates, result));
  });
};
