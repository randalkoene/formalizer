# Installation Notes

## Off-site database backups: restic and rclone

The Formalizer database backups in `~/.formalizer/archive/` are copied off-site,
encrypted, with `restic`, which here stores its repository on Google Drive through
`rclone`. Both are installed by `tools/system/replicate/replicate.py` (essential
programs), or by hand with `sudo apt install restic rclone`.

Setting this up for a new Formalizer installation, and getting the backups back on a
new machine, is described step by step in `core/fzbackup/README.md`, section
"Setting up encrypted off-site backups (restic and rclone)".

What the off-site copy depends on:

| Item | Location | Comes back with a restored home directory? |
|---|---|---|
| rclone remote `gdrive` | `~/.config/rclone/rclone.conf` | yes |
| restic password | `~/.config/restic/formalizer.pass` (mode 600), plus a copy kept off the machine | yes |
| restic repository | `rclone:gdrive:formalizer-restic` | (it is off-site) |

After restoring a home directory from the external-drive backup (replicate.py),
check that the off-site copy is still reachable with:

```
restic -r rclone:gdrive:formalizer-restic --password-file ~/.config/restic/formalizer.pass snapshots
```

## Interference by browser extensions

Browser extensions that examine or filter http calls may interfere with or
slow down Formalizer web operation. If certain pages appear to open very
slowly, add the Formalizer server address(es) to the white list of the
extension involved (e.g. uBlock Origin Lite). For example:

```
localhost
127.0.0.1
```

## Slowdown / hang of calls to localhost:8090

It seems that Chrome may cause hangs that resolve after about a minute
when it does DNS related work on localhost URLs.

A fairly simple solution may be to use 127.0.0.1 directly instead of
localhost when opening the Formalizer top page, Nodes page, Log page,
etc. More information is provided in Log chunk 202606201025 and the
document linked from there.

## Access permissions for web control directories

Web-based Formalizer control depends on activities carried out in the
browser, i.e. by www-data, as well as activities carried out by the user
running the fzserverpq (e.g. randalk).

For this to work, there need to be directories that are writable by one
of these users and readable by the other, and vice-versa. The standard
is this:

/var/www/html/formalizer is owned by USER:USER (e.g. randalk)
/var/www/webdata is owned by www-data:www-data

This rule is also important to remember when creating utilities, e.g.
scripts. A script that is typically run by the user (e.g. periodically
in the background by crontab or on the command line in a terminal)
should write any data that is used by web-based operations to
/var/www/html/formalizer. A script that is typically run as a CGI
script should write any data that is used by other web-based operations
or by user commands to /var/www/webdata.
