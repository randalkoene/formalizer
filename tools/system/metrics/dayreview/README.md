# dayreview

The `dayreview.py` script is a temporary helper script used to develop
automation for a post-hoc review of a day's activities.

## Scripts

- `dayreview.py`: Command line script that uses JSON output from fzloghtml for review.
- `dayreview-cgi.py`: Web script that uses FORM submission after HTML output from fzloghtml for review.
- `dayreview-plot(-cgi).py`: Scripts that generate data statistics for day reviews.

Both `dayreview.py` and `dayreview-cgi.py` use `dayreview_algorithm.py`.

--
Randal A. Koene, 20240222
