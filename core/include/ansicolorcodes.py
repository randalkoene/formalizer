# Copyright 2020 Randal A. Koene
# License TBD

"""
This header file declares ANSI color codes for use in print statements
on compatible terminal consoles.

Versioning is based on https://semver.org/. See coreversion.hpp for more.
"""

ANSI_nrm = '\u001b[0m'          # reset to normal (also turns off bright)
ANSI_alert = '\u001b[31;1m'     # alert color (bright red)
ANSI_warn = '\u001b[31m'        # warning color (dark red)

ANSI_wt = '\u001b[38;5;15m'     # white
ANSI_wb = '\u001b[37;1m'        # white bright
ANSI_gd = '\u001b[32m'          # green slightly darker
ANSI_gn = '\u001b[38;5;47m'     # green
ANSI_gb = '\u001b[32;1m'        # green bright
ANSI_rd = '\u001b[38;5;202m'    # red orange salmon
ANSI_yl = '\u001b[33m'          # yellow
ANSI_yb = '\u001b[33;1m'        # yellow bright
ANSI_bl = '\u001b[34m'          # blue
ANSI_bb = '\u001b[34;1m'        # blue bright
ANSI_lb = '\u001b[38;5;33m'     # blue light
ANSI_mg = '\u001b[35m'          # magenta
ANSI_mb = '\u001b[35;1m'        # magenta bright
ANSI_cy = '\u001b[36m'          # cyan
ANSI_cb = '\u001b[36;1m'        # cyan bright
ANSI_pu = '\u001b[38;5;57m'     # purple
ANSI_lv = '\u001b[38;5;69m'     # lavender
ANSI_br = '\u001b[38;5;136m'    # brown
ANSI_lt = '\u001b[38;5;138m'    # leather
ANSI_or = '\u001b[38;5;208m'    # orange
ANSI_dg = '\u001b[38;5;241m'    # dark gray
ANSI_gg = '\u001b[38;5;244m'    # medium gray
ANSI_lg = '\u001b[38;5;249m'    # light gray
ANSI_bk = '\u001b[30m'          # black

ANSI_RD = '\u001b[47m'          # background white
ANSI_RB = '\u001b[47;1m'        # background white bright
ANSI_RD = '\u001b[41m'          # background red dark
ANSI_RB = '\u001b[41;1m'        # background red bright
ANSI_GD = '\u001b[42m'          # background green dark
ANSI_GB = '\u001b[42;1m'        # background green bright
ANSI_YD = '\u001b[43m'          # background yellow dark
ANSI_YB = '\u001b[43;1m'        # background yellow bright
ANSI_BD = '\u001b[44m'          # background blue dark
ANSI_BB = '\u001b[44;1m'        # background blue bright
ANSI_MD = '\u001b[45m'          # background magenta dark
ANSI_MB = '\u001b[45;1m'        # background magenta bright
ANSI_CD = '\u001b[46m'          # background cyan dark
ANSI_CB = '\u001b[46;1m'        # background cyan bright
ANSI_BK = '\u001b[40m'          # background black
ANSI_BG = '\u001b[40;1m'        # background gray


def Yes_no(ansi_from):
    return f'{ANSI_gn}Y{ansi_from}/{ANSI_rd}n{ansi_from}'


def No_yes(ansi_from):
    return f'{ANSI_rd}y{ansi_from}/{ANSI_gn}N{ansi_from}'
