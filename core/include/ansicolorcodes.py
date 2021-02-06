# Copyright 2020 Randal A. Koene
# License TBD

"""
This header file declares ANSI color codes for use in print statements
on compatible terminal consoles.

Versioning is based on https://semver.org/. See coreversion.hpp for more.
"""

ANSI_nrm = '\u001b[32m' # reset to normal
ANSI_alert = '\u001b[31m' # alert color
ANSI_wt = '\u001b[38;5;15m' # white
ANSI_wb = '\u001b[37;1m' # bright white
ANSI_gn = '\u001b[38;5;47m' # green
ANSI_rd = '\u001b[38;5;202m' # red
ANSI_yl = '\u001b[33m' # yellow
ANSI_yb = '\u001b[33;1m' # bright yellow
ANSI_bl = '\u001b[34m' # blue
ANSI_bb = '\u001b[34;1m' # bright blue
ANSI_lb = '\u001b[38;5;33m' # red
ANSI_mg = '\u001b[35m' # magenta
ANSI_mb = '\u001b[35;1m' # bright magenta
ANSI_cy = '\u001b[36m' # cyan
ANSI_cb = '\u001b[36;1m' # bright cyan
ANSI_pu = '\u001b[38;5;57m' # purple
ANSI_lv = '\u001b[38;5;69m' # lavender
ANSI_br = '\u001b[38;5;136m' # brown
ANSI_lt = '\u001b[38;5;138m' # leather
ANSI_or = '\u001b[38;5;208m' # orange
ANSI_dg = '\u001b[38;5;241m' # dark gray
ANSI_gg = '\u001b[38;5;244m' # medium gray
ANSI_lg = '\u001b[38;5;249m' # light gray


def Yes_no(ansi_from):
    return f'{ANSI_gn}Y{ansi_from}/{ANSI_rd}n{ansi_from}'


def No_yes(ansi_from):
    return f'{ANSI_rd}y{ansi_from}/{ANSI_gn}N{ansi_from}'
