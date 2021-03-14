# Import this as a module to use the Task Chunk state-machine signal API.
# This module should be symlinked to core/include for easy import into
# other Formalizer Python programs.
#
# Task Chunk Signal API:
#
# /ping
#   Ping the server connection.
#
# /start (t, mins)
#   A new Task Chunk was started at t with a duration of mins minutes.
#
# /end (t)
#   A Task Chunk was ended at t.

from dataclasses import dataclass
import time
import requests
from requests.exceptions import HTTPError, Timeout

# This is an example of how an event/signal can happen that changes Task Chunk state.
# Task managers, such as fztask.py, can produce such signals.

# +---- FZ SSE server class
@dataclass
class FZTaskServer:
    """Use this to share configured settings for communications with the SSE server."""
    address: str = 'localhost:5000'
    verbosity: int = 1      # 0 = quiet, 1 = verbose, 2 = very verbose
    timeout_seconds: float = 0.05
    response = None

    def get(self, urlpath: str, params = None) -> bool:
        try:
            self.response = requests.get(f'http://{self.address}{urlpath}', params=params, timeout = self.timeout_seconds)
            self.response.raise_for_status()
            return True
        except HTTPError as http_err:
            if (self.verbosity > 0):
                print(f'HTTP error occurred: {http_err}')
        except Timeout as timeout_err:
            if (self.verbosity > 0):
                print(f'Timeout error occurred: {timeout_err}')
        except Exception as err:
            if (self.verbosity > 0):
                print(f'An error occurred: {err}')
        return False       

    # +---- Sending state change signals

    def ping(self) -> bool:
        return self.get('/ping')

    # @param fztaskserveraddr A complete address, e.g. localhost:5000.
    # @param t_start A Formalizer time stamp string for the Task Chunk start.
    # @param minutes The duration of the Task Chunk in minutes.
    def start_task_chunk(self, t_start: str, minutes: int) -> bool:
        chunk_start_data = { 't': t_start, 'minutes': str(minutes) }
        return self.get('/start', params=chunk_start_data)

    def end_task_chunk(self, t_end: str) -> bool:
        chunk_end_data = { 't': t_end }
        return self.get('/end', params=chunk_end_data)

    # +---- Making broadcast requests

    # +---- Listening to subscriptions

# +---- API end

# When called as the __main__ script, simply run the ping test.
if __name__ == '__main__':

    server = FZTaskServer()
    while True:
        server.ping()
        time.sleep(1)
