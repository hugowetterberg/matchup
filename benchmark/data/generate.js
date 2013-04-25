var lib = {
  fs: require('fs'),
  crypto: require('crypto')
};

var key, region_list, questionnaire;

region_list = [
  {name:"a", candidates: 200}
];

questionnaire = [
  {answers:[0, 25, 75, 100]},
  {answers:[0, 50, 100]},
  {answers:[0, 50, 100]},
  {answers:[0, 25, 75, 100]},
  {answers:[0, 75, 100]},
  {answers:[0, 25, 75, 100]},
  {answers:[0, 50, 100]},
  {answers:[0, 50, 100]},
  {answers:[0, 25, 75, 100]},
  {answers:[0, 75, 100]},
  {answers:[0, 25, 75, 100]},
  {answers:[0, 75, 100]},
  {answers:[0, 50, 100]},
  {answers:[0, 25, 75, 100]},
  {answers:[0, 25, 75, 100]},
  {answers:[0, 50, 100]},
  {answers:[0, 50, 100]},
  {answers:[0, 50, 100]},
  {answers:[0, 25, 75, 100]},
  {answers:[0, 25, 75, 100]}
];

region_list.forEach(function(region, region_idx) {
  var
    i, current, selection, generate, out,
    fileHeaderSize = 4, candidateHeaderSize = 4,
    data = new Buffer(fileHeaderSize + (region.candidates * (candidateHeaderSize + questionnaire.length)));

  // Write a our header with candidate and question count
  data.writeUInt16LE(region.candidates, 0);
  data.writeUInt16LE(questionnaire.length, 2);

  generate = function(index, data, callback) {
    var offset = 0;

    // Write a candidate id (UInt32LE composed of candidate index and
    // region index)
    data.writeUInt16LE(index, offset);
    data.writeUInt16LE(region_idx, offset+2);
    offset += 4;

    // Write random choices for the questions
    var selection = lib.crypto.randomBytes(questionnaire.length);
    for (i in questionnaire) {
      // Write a byte for our selection
      data.writeInt8(questionnaire[i].answers[selection[i] % questionnaire[i].answers.length], offset++);
    }
  };

  for (current = 0; current < region.candidates; current++) {
    generate(current, data.slice(fileHeaderSize + (current * (candidateHeaderSize + questionnaire.length))));
  }

  // Write out our data
  out = lib.fs.createWriteStream(region.name + '.bin');
  out.write(data, function() {
    out.close();
  });
});
