# Import this as a module to use the Task Chunk state-machine signal API.
# This module should be symlinked to core/include for easy import into
# other Formalizer Python programs.
#
# Task Chunk Signal API:
#
# /start (t, mins)
#   A new Task Chunk was started at t with a duration of mins minutes.
#
# /end (t)
#   A Task Chunk was ended at t.

import time
import requests

# This is an example of how an event/signal can happen that changes Task Chunk state.
# Task managers, such as fztask.py, can produce such signals.

# @param fztaskserveraddr A complete address, e.g. localhost:5000.
# @param t_start A Formalizer time stamp string for the Task Chunk start.
# @param minutes The duration of the Task Chunk in minutes.
def start_task_chunk(fztaskserveraddr: str, t_start: str, minutes: int) -> bool:
    try:
        print(f'DEBUG: addr={fztaskserveraddr}, t_start={t_start}, minutes={minutes}')
        requests.get(f'http://{fztaskserveraddr}/start?t={t_start}&mins={str(minutes)}')
        return True
    except:
        return False


def end_task_chunk(fztaskserveraddr: str, t_end: str) -> bool:
    try:
        requests.get(f'http://{fztaskserveraddr}/end?t={t_end}')
        return True
    except:
        return False


# When called as the __main__ script, simply run the ping test.
if __name__ == '__main__':

    while True:
        requests.get('http://localhost:5000/ping')
        time.sleep(1)
