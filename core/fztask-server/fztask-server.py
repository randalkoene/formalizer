import sys
import queue

import flask
import flask_cors

import json
from dataclasses import dataclass


# This is the little SSE server that can curate and arbitrate task chunk management
# states. It should define a little state machine for the Task Chunk state. A
# manager such as fztask.py or a browser-based javascript manager can implement
# a corresponding little state machine that requests or reacts to changes in
# Task Chunk state.
#
# You can make this visible via all addresses to this machine by specifying the
# special address '0.0.0.0'. You can either provide that in app.run(host='0.0.0.0')
# or via 'flask run --host 0.0.0.0'.
#
# Note though: You really should not run the Flask server in DEVELOPMENT mode
# when exposed beyond localhost. See the following link for information about
# running a Flask server in a production environment:
# http://flask.pocoo.org/docs/quickstart/#a-minimal-application

app = flask.Flask(__name__)
flask_cors.CORS(app)

ReferenceTime = {
    'RTt_unspecified' : -1.0
}

chunk_mins_default = 20

# A little Task (Log) Chunk state machine
@dataclass
class FZTaskState:
    active: bool = False            # is a task active (i.e. Log chunk open)?
    active_confirmed: bool = False  # has 'active' state been confirmed from the Log?
    t_open: str = ''                # last known chunk open time
    t_open_confirmed: bool = False
    t_close: str = ''               # last known chunk close time
    t_close_confirmed: bool = False
    duration_mins: int = chunk_mins_default # intended duration of active chunk

taskstate = FZTaskState()

@app.route('/')
@flask_cors.cross_origin()
def server_status():
    return 'The FZ Task Server is listening.'

# This class provides subscription services.
# Every subscriber receives a queue of size maxsize.
class MessageAnnouncer:

    def __init__(self):
        self.listeners = []

    # Accept new subscribers and return the queue for the newest one.
    # The queue objects are thread-safe.
    def listen(self):
        self.listeners.append(queue.Queue(maxsize=5))
        return self.listeners[-1]

    # Broadcast published messages to subscribers by putting the message into their
    # respective queues.
    def announce(self, msg):
        # We go in reverse order because we might have to delete an element, which will shift the
        # indices backward
        for i in reversed(range(len(self.listeners))):
            try:
                # The non-blocking queue put_nowait() method raises an exception if queue is Empty or Full.
                self.listeners[i].put_nowait(msg)
            except queue.Full:
                # We assume that a full queue means the subscriber has disappeared and is no longer reading messages.
                del self.listeners[i]


announcer = MessageAnnouncer()


def format_sse(data: str, event=None) -> str:
    """Formats a string and an event name in order to follow the event stream convention.

    >>> format_sse(data=json.dumps({'abc': 123}), event='Jackson 5')
    'event: Jackson 5\\ndata: {"abc": 123}\\n\\n'

    """
    msg = f'data: {data}\n\n'
    if event is not None:
        msg = f'event: {event}\n{msg}'
    return msg


# The ping-pong route can be used to test the SSE client-server connection.
@app.route('/ping')
@flask_cors.cross_origin()
def ping():
    msg = format_sse(data='pong')
    announcer.announce(msg=msg)
    return {}, 200


# Subscribe to SSE messages
@app.route('/listen', methods=['GET'])
@flask_cors.cross_origin()
def listen():

    # Coroutine that yields its excution upon each message read and will continue
    # with the next while iteration when called again.
    # This function becomes the request method in the server's ongoing Response()
    # connection with the client.
    # As mentioned at https://blog.miguelgrinberg.com/post/customizing-the-flask-response-class,
    # Flask typically uses flask.Responnse() internally as a container for the response data
    # returned by application route functions, plus some additional information needed to
    # create an HTTP response.
    def stream():
        messages = announcer.listen()  # returns a queue.Queue
        while True:
            msg = messages.get()  # blocks until a new message arrives
            yield msg

    return flask.Response(stream(), mimetype='text/event-stream')


# Recieve a TC start signal and update taskstate, broadcast taskstate
@app.route('/start')
@flask_cors.cross_origin()
def TC_start():
    if flask.request.args.get('t'):
        t_start = flask.request.args.get('t')
    else:
        return 'bad request', 400
    if flask.request.args.get('mins'):
        mins = flask.request.args.get('mins')
    else:
        return 'bad request', 400

    taskstate.t_open = t_start
    taskstate.duration_mins = int(mins)
    taskstate.active = True

    msg = format_sse(data=json.dumps({'t': t_start, 'mins': mins}), event='TC_start')
    announcer.announce(msg=msg)
    #return {}, 200
    response = flask.make_response('TC start announced', 200)
    response.headers['Content-Type'] = 'application/json'
    return response


# Recieve a TC end signal and update taskstate, broadcast taskstate
@app.route('/end')
@flask_cors.cross_origin()
def TC_end():
    if flask.request.args.get('t'):
        t_end = flask.request.args.get('t')
    else:
        return 'bad request', 400

    taskstate.t_close = t_end
    taskstate.active = False

    msg = format_sse(data=json.dumps({'t': t_end}), event='TC_end')
    announcer.announce(msg=msg)
    #return {}, 200
    response = flask.make_response('TC end announced', 200)
    response.headers['Content-Type'] = 'application/json'
    return response


# Receive a TC state request, broadcast taskstate
@app.route('/state')
@flask_cors.cross_origin()
def TC_state():
    active = taskstate.active
    if active:
        tstamp = taskstate.t_open
        mins = taskstate.duration_mins
        msg = format_sse(data=json.dumps({'state': active, 't': tstamp, 'mins': mins}), event='TC_state')
    else:
        tstamp = taskstate.t_close
        msg = format_sse(data=json.dumps({'state': active, 't': tstamp}), event='TC_state')

    announcer.announce(msg=msg)
    #return {}, 200
    response = flask.make_response('TC state announced', 200)
    response.headers['Content-Type'] = 'application/json'
    return response


if __name__ == '__main__':

    app.debug = True
    app.run(host = '0.0.0.0', port=5000)
    sys.exit(0)
