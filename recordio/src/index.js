const read = require("@dcos/recordio").read
var http = require('http')

var input = http.get('http://localhost:1337/my_process/json', (res) => {
  let rawData = '';
  res.on('data', (chunk) => {
    //const [records, rest] = read(chunk);
    //rawData = records[0];
    rawData += chunk;
  });
  res.on('end', () => {
    try {
      //const parsedData = rawData;
      var start = performance.now();
      document.querySelector('pre').textContent = JSON.parse(rawData);
      var stop = performance.now();
      //console.log(parsedData);
      //return rawData;
    } catch (e) {
      console.error(e.message);
    }
  });
});

//document.querySelector('pre').textContent = input;//JSON.stringify(JSON.parse(input),null,2);

/*
var input = http.get('http://localhost:1337/my_process/json')
const [records, rest] = read(input);
*/

