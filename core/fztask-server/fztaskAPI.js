// subscribe to Task Chunk messages
var source = new EventSource('http://{{ fztaskserveraddr }}/listen');

// +---- Listening to subscriptions

// handle messages
source.onmessage = function(event) {
    // Do something with the data:
    console.log(event.data);
};

// Subscriber Listens for TC_start event
source.addEventListener("TC_start", function(event) {
    const t = JSON.parse(event.data).t
    const mins = JSON.parse(event.data).mins
    console.log('TC_start t = '+ t + ', mins = ' + mins)
});

// Subscriber Listens for TC_end event
source.addEventListener("TC_end", function(event) {
    const t = JSON.parse(event.data).t
    console.log('TC_end t = '+ t)
});

// +---- Making broadcast requests

// +---- Sending state change signals

// Could add connection stop detection as in https://medium.com/conectric-networks/a-look-at-server-sent-events-54a77f8d6ff7
