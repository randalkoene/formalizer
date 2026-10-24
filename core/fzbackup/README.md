# fzbackup - Formalizer core utility to back up the Formalizer environment

By default, the `fzbackup` core utility makes a backup of the database that contains
the Graph, Log, Named Nodes Lists, and such.

### How to provide a button on an HTML page that runs fzbackup as the right user

1. The button on the HTML page calls a CGI script in a new window (`target="_blank"`).

2. The CGI script makes a FZ API call to `fzserverpq`, which is running as the right user.

3. That call is a predefined permitted System call that launches the `fzbackup` process
   in the predefined manner, e.g. via `fzbackup-mirror-to-github.sh`, as a background
   process, so that it does not keep `fzserverpq` busy.

4. The background process writes its output to a known file.

5. The original CGI script waited around for that file to appear, possibly even writing
   content to the new HTML page as it works.

6. The output of the backup process appears on the new HTML page.

## `backup-environment.py` - Backup the whole account

The regular full backup is made when the regularly scheduled Node 20201002064246.1
comes up (there is a button for it under "Backups" on the FZ Admin page). Run:

```
backup-environment -full
```

This shows the current settings (external drive mountable partition file, its mount
point, the directories to rsync, dry-run mode) and a menu of options. The two main ones
are:

- `b` = Proceed with backup. Loop-mounts the mountable partition file (an ext4 file
  system inside a file on the external drive) at the mount point (needs `sudo`), then
  runs `rsync --delete --force -avhP` for each directory in the list: `/home/randalk`,
  `/var/www` and `/usr/lib/cgi-bin`. All of `/home/randalk` is copied, including hidden
  directories such as `~/.formalizer` and `~/.claude` (Claude Code settings, memory and
  session transcripts). When rsync is done it asks whether to unmount (default yes).
  Toggle dry-run mode with `d` first to see what would be copied without copying.

- `c` = Create mountable partition. Only for setting up a new backup drive or partition
  file: the file must not exist yet and the mount point must already exist (make it with
  `M`). Creates a 180G file at the external path, formats it as ext4, mounts it to
  `chmod -R 777` it, and unmounts it. After that, use `b` as usual. If the path of the
  partition file or mount point changes, update the defaults in the `config` dictionary
  at the top of the script (or change them for one run with `e` and `m`).



--
Randal A. Koene 2023
