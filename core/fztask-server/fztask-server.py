import sys
import queue

import flask
import flask_cors

import json


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


@app.route('/')
@flask_cors.cross_origin()
def server_status():
    return 'The FZ Task Server is listening.'


class MessageAnnouncer:

    def __init__(self):
        self.listeners = []

    def listen(self):
        self.listeners.append(queue.Queue(maxsize=5))
        return self.listeners[-1]

    def announce(self, msg):
        # We go in reverse order because we might have to delete an element, which will shift the
        # indices backward
        for i in reversed(range(len(self.listeners))):
            try:
                self.listeners[i].put_nowait(msg)
            except queue.Full:
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

    def stream():
        messages = announcer.listen()  # returns a queue.Queue
        while True:
            msg = messages.get()  # blocks until a new message arrives
            yield msg

    return flask.Response(stream(), mimetype='text/event-stream')


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
    msg = format_sse(data=json.dumps({'t': t_start, 'mins': mins}), event='TC_start')
    announcer.announce(msg=msg)
    #return {}, 200
    response = flask.make_response('TC start announced', 200)
    response.headers['Content-Type'] = 'application/json'
    return response


@app.route('/end')
@flask_cors.cross_origin()
def TC_end():
    if flask.request.args.get('t'):
        t_end = flask.request.args.get('t')
    else:
        return 'bad request', 400
    msg = format_sse(data=json.dumps({'t': t_end}), event='TC_end')
    announcer.announce(msg=msg)
    #return {}, 200
    response = flask.make_response('TC end announced', 200)
    response.headers['Content-Type'] = 'application/json'
    return response


if __name__ == '__main__':

    app.debug = True
    app.run(host = '0.0.0.0', port=5000)
    sys.exit(0)
