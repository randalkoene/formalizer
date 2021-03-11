// subscribe to Task Chunk messages
var source = new EventSource('http://{{ fztaskserveraddr }}/listen');

// handle messages
source.onmessage = function(event) {
    // Do something with the data:
    console.log(event.data);
};

source.addEventListener("TC_start", function(event) {
    const t = JSON.parse(event.data).t
    const mins = JSON.parse(event.data).mins
    console.log('TC_start t = '+ t + ', mins = ' + mins)
});

source.addEventListener("TC_end", function(event) {
    const t = JSON.parse(event.data).t
    console.log('TC_end t = '+ t)
});

// Could add connection stop detection as in https://medium.com/conectric-networks/a-look-at-server-sent-events-54a77f8d6ff7
