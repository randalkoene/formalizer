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

- `v` = Verify the copy on the external drive (also offered right after `b`, and
  available directly as `backup-environment -verify`). It mounts the partition if
  needed and then:
  1. counts files in each rsync directory and in its copy;
  2. compares the directories in `checksumdirs` (by default `~/.formalizer` and
     `~/src/formalizer`) with their copies by checksum (an `rsync --checksum` dry run);
  3. runs `fzbackup-verify.py` on the newest database backup in the copy.

  Small differences in counts and checksums are expected for files that changed after
  the rsync.

## `fzbackup-verify.py` - Check that a database backup can be restored

```
fzbackup-verify.py [backup.gz | directory] [--fresh] [--keep] [-d formalizer]
```

Tests a gzipped `pg_dump` backup (by default the newest in the `backuproot` of the
fzbackup config) without touching the live database: `gzip -t`, SHA-256, a restore into a
scratch database `fzverify_<timestamp>` with `ON_ERROR_STOP` in a single transaction, and
a comparison of the row count of every table with the live database. The scratch database
is dropped afterwards unless `--keep` is given. `--fresh` makes a new backup with
`fzbackup` first, so that the comparison is against the current state.

Exit codes: 0 = restored and all tables match; 1 = a check failed (damaged file, restore
error, no tables); 2 = restored, but row counts differ from the live database, which is
expected when the Formalizer changed after the backup was made.

## `fzbackup-prune.py` - Thin out database backups

```
fzbackup-prune.py [directory] [--apply] [--keep-last 4] [--daily-days 14] [--weekly-days 92] [-q]
```

Keeps the newest backup of each day for 14 days, of each ISO week for 3 months, and of
each month forever, plus the 4 most recent backups. Everything else in the backup
directory that matches `formalizer-postgres-backup-YYYYmmddHHMM.gz` is deleted. Every
month keeps at least one backup. Without `--apply` it only lists what it would keep (and
why) and what it would delete.

## Setting up encrypted off-site backups (restic and rclone)

Local backups in `~/.formalizer/archive/` do not survive the loss of the machine. The
off-site copy uses [restic](https://restic.net), which encrypts everything before it
leaves the machine, removes duplicate data between backups, and applies the same kind of
retention rule as `fzbackup-prune.py`. Restic can store its repository in many places.
This guide uses Google Drive, reached through [rclone](https://rclone.org). A shorter
alternative without rclone is at the end.

Do this once per Formalizer installation. It takes about half an hour, most of it in the
Google Cloud console. Words in CAPITALS are your own choices.

### 1. Install

```
sudo apt install restic rclone
```

(`tools/system/replicate/replicate.py` installs both as essential programs.)

### 2. Make your own Google OAuth client

rclone needs permission to use your Google Drive. Do not use rclone's built-in client:
it is shared by all rclone users and is usually over Google's quota (rclone then fails
with `Error 403: Quota exceeded ... rateLimitExceeded`).

In https://console.cloud.google.com, signed in with the Google account whose Drive will
hold the backups:

1. Create a project (for example `formalizer-backup`) and select it.
2. APIs & Services, Library: enable the **Google Drive API**.
3. Google Auth Platform (older menus: OAuth consent screen): give an app name and your
   email as support and contact address, and choose audience **External**. Do not upload
   a logo: a logo makes Google require a review of the app.
4. Google Auth Platform, Clients, Create client: type **Desktop app**, any name (for
   example `rclone-HOSTNAME`), leave "AI-powered agent" unchecked. Copy the client ID and
   client secret.
5. Publishing. While the app's status is "Testing", Google ends the sign-in after
   **7 days**, so unattended backups stop working a week later. To publish (Audience,
   Publish app) Google requires a home page URL and a privacy policy URL on the Branding
   page, on a domain listed there under Authorized domains. A short page on a website you
   control is enough, for example: "This app is used only by NAME to back up NAME's own
   files to NAME's own Google Drive. It collects and shares no data." Because the app only
   asks for access to the files it creates, publishing needs no review by Google.

   Until it is published, renew the sign-in weekly with `rclone config reconnect gdrive:`.
   Adding yourself as a test user is not needed to sign in, and Google may refuse it for
   some accounts.

Keep a note of the Google account, project and client name in your own records (not in
this repository).

### 3. Connect rclone to Google Drive

```
rclone config
```

| Prompt | Answer |
|---|---|
| `n/s/q>` | `n` (new remote). Skip `s`: a config password would stop unattended runs. |
| `name>` | `gdrive` |
| `Storage>` | `drive` |
| `client_id>`, `client_secret>` | the values from step 2 |
| `scope>` | "Access to files created by rclone only" (`drive.file`) |
| `root_folder_id>`, `service_account_file>` | Enter |
| `Edit advanced config?` | `n` |
| `Use auto config?` | `y`. Sign in with the right Google account. At "Google hasn't verified this app": Advanced, Go to APP NAME, Allow. |
| `Configure this as a team drive?` | `n`, then `y` to keep the remote, then `q` |

Check: `rclone lsd gdrive:` prints nothing and no error (rclone only sees files it has
made). `rclone about gdrive:` shows the free space.

### 4. Make the restic password

```
mkdir -p ~/.config/restic && chmod 700 ~/.config/restic
( umask 077; openssl rand -base64 33 > ~/.config/restic/formalizer.pass )
```

Store a copy of this password somewhere that does not depend on this machine (a password
manager, or on paper in a safe place). **Without it, the off-site backups can never be
read.** To see it: `cat ~/.config/restic/formalizer.pass`.

### 5. Create the repository and make the first backup

```
export RESTIC_REPOSITORY=rclone:gdrive:formalizer-restic
export RESTIC_PASSWORD_FILE=~/.config/restic/formalizer.pass
restic init
restic backup --tag formalizer-archive ~/.formalizer/archive
restic snapshots
```

### 6. Check that the backup can be read back

```
restic check
restic restore latest --target /tmp/fzrestore-test
fzbackup-verify.py /tmp/fzrestore-test$HOME/.formalizer/archive/postgres
rm -rf /tmp/fzrestore-test
```

`fzbackup-verify.py` restores the newest database backup from that copy into a scratch
database and compares it with the live database.

### Getting backups back on a new machine

Install restic and rclone, redo step 3 (or restore `~/.config/rclone/rclone.conf`), put
the password back into `~/.config/restic/formalizer.pass`, and run:

```
restic -r rclone:gdrive:formalizer-restic --password-file ~/.config/restic/formalizer.pass \
    restore latest --target /tmp/fzrestore
```

The database backups are then under `/tmp/fzrestore$HOME/.formalizer/archive/postgres/`.
Check one with `fzbackup-verify.py FILE` before restoring it with `fzrestore.sh`.

### Alternative: Backblaze B2 without rclone

Restic can talk to Backblaze B2 directly: no OAuth client, no consent screen and no
expiring sign-in, only an account and an application key (the first 10 GB are free).
Make a bucket and an application key in the B2 web console, then use
`RESTIC_REPOSITORY=b2:BUCKET:formalizer-restic` with `B2_ACCOUNT_ID` and `B2_ACCOUNT_KEY`
set, and follow steps 4 to 6.



--
Randal A. Koene 2023
