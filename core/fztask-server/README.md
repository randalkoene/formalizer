## Flask SSE example, modified by Randal A. Koene 2021-03-10

> Look ma, no dependencies!

This repository is an example of how to perform [server-sent events (SSE)](https://www.wikiwand.com/en/Server-sent_events) in Flask with no extra dependencies. Libraries such as [flask-sse](https://github.com/singingwolfboy/flask-sse) are great, but they require having to use Redis or some other sort of [pubsub](https://www.wikiwand.com/en/Publish%E2%80%93subscribe_pattern) backend. While this can be fine, I wanted to show that you can do SSE by only using Flask. The following instructions explain how to run the example. For more information on how this works, please see [the accompanying blog post](https://maxhalford.github.io/blog/flask-sse-no-deps).


### To Test

You're going to need 3 terminal sessions plus a test page open on any browser you intend to test to run this example in it's entirety. The first will run the server, the second will listen to events, and the third will trigger events. You may also open an additional terminal if you want to add another listener. In fact, having multiple listeners is a good test to check that a single message is correctly dispatched once to each listener.

In the first terminal, create a virtual environment, activate it, and install the necessary requirements.

```sh
python3 -m venv env
source env/bin/activate
pip install -r requirements.txt
```

Next, run the server.

```sh
export FLASK_APP=fztask-server.py
export FLASK_ENV=development
flask run
```

In the second terminal, listen for updates.

```sh
source env/bin/activate
python fztask-client.py
```

Open any browsers you wish to test to the `test.html` page. This can be a URL
such as `file:///home/randalk/src/formalizer/core/fztask-server/test.html`.
Or, you can copy the `test.html` file to `/var/www/html/formalizer/` and
load that page as `http://localhost/formalizer/test.html`.
Or, you can open a browser on another computer that can reach the same network,
for example, `http://morpheus.local/formalizer/test.html`.

Finally, in the third terminal, run the `fztaskAPI.py` script.

```sh
source env/bin/activate
python fztaskAPI.py
```

This will run an infinite loop which sends a GET request to the `/ping` route every second. This in turn will trigger an event which will be displayed in the listening terminal. You should see a new `pong` message being displayed every second or so.

Once you know that the ping-pong test is working then you can go ahead and implement signals (by importing `fztaskAPI.py`) and responses (as in `fztaskAPI.js`) to actual Task Chunk events such as TC_start and TC_end.
