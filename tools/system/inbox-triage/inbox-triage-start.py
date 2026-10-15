#!/usr/bin/env python3
"""Start the inbox-triage web server in the background, detached, as you.

Meant to be run by fzserverpq on behalf of the dashboard button, through

  FZ CGIbg_run_as_user(inbox-triage-start.py,,/dev/shm/inbox-triage-start.out)

which only works once inbox-triage-start.py is listed in the fzserverpq
configuration variable 'predefined_CGIbg'. Running it by hand is fine too.

fzserverpq does not open its sockets close-on-exec, so anything it starts
inherits them -- including its listening socket on port 8090. Short-lived
programs let go of those within seconds; a server that runs for hours would
keep fzserverpq from binding 8090 again after a restart. So this closes every
inherited descriptor and starts serve.py in a session of its own.

Arguments arrive in fzserverpq's converted URI form, e.g. "idle_exit=60"
becomes "-idle_exit 60".

  inbox-triage-start.py                 # idle exit after 180 minutes
  inbox-triage-start.py -idle_exit 0    # never idle-exit

Prints one line saying what happened; serve.py's own output goes to
cache/serve.log.
"""

import argparse
import os
import socket
import sys
import time

HERE = os.path.dirname(os.path.realpath(__file__))
SERVE = os.path.join(HERE, "serve.py")
LOG = os.path.join(HERE, "cache", "serve.log")
HOST = "127.0.0.1"
PORT = 8711
START_TIMEOUT_S = 20


def listening():
    try:
        with socket.create_connection((HOST, PORT), timeout=0.5):
            return True
    except OSError:
        return False


def tail(path, n=15):
    try:
        with open(path, errors="replace") as fh:
            return "".join(fh.readlines()[-n:])
    except OSError:
        return ""


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("-idle_exit", type=int, default=180, metavar="MINUTES")
    args = parser.parse_args()

    if listening():
        print("inbox-triage already running on port %d" % PORT)
        return 0

    os.makedirs(os.path.dirname(LOG), exist_ok=True)
    log = open(LOG, "ab")
    log.write(("\n--- %s  started by inbox-triage-start\n"
               % time.strftime("%Y-%m-%d %H:%M:%S")).encode())
    log.flush()

    pid = os.fork()
    if pid == 0:
        os.setsid()
        devnull = os.open(os.devnull, os.O_RDONLY)
        os.dup2(devnull, 0)
        os.dup2(log.fileno(), 1)
        os.dup2(log.fileno(), 2)
        os.closerange(3, os.sysconf("SC_OPEN_MAX"))
        os.chdir(HERE)
        env = dict(os.environ, PYTHONUNBUFFERED="1")
        os.execve(sys.executable,
                  [sys.executable, SERVE, "--no-browser",
                   "--idle-exit", str(max(0, args.idle_exit))],
                  env)

    log.close()
    for _ in range(START_TIMEOUT_S * 10):
        time.sleep(0.1)
        if listening():
            print("inbox-triage started, pid %d, http://%s:%d/" % (pid, HOST, PORT))
            return 0
        done, status = os.waitpid(pid, os.WNOHANG)
        if done:
            print("inbox-triage exited during startup (status %d). End of %s:\n%s"
                  % (os.waitstatus_to_exitcode(status), LOG, tail(LOG)))
            return 1
    print("inbox-triage did not start listening within %d s. End of %s:\n%s"
          % (START_TIMEOUT_S, LOG, tail(LOG)))
    return 1


if __name__ == "__main__":
    sys.exit(main())
