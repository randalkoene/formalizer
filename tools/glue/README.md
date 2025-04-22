# Glue utilities

## fzcrontab and fzdaily.sh

For things that should be done regularly to maintain a properly up-to-date
Formalizer environment.

The `fzcrontab` file contains the expression to load into crontab as follows:

```
crontab ./fzcrontab
```

This will enable a cron job that regularly calls `fzdaily.sh`.

The `fzdaily.sh` script will compare a new date stamp with one that was
stored in the file `~/.formalizer/fzdaily.stamp`. The date stamp ensures
that - even if the cron job is launched more than once per day to ensure
it is run - the daily tasks will **only run once per day**.

It will then carry out the tasks specified in `fzdaily_do.source`.

These include tasks such as:

- Backing up the database with `fzbackup`.
- Refreshing the Node histories with `fzquerypq`.

## Proposed additional regularly scheduled tasks

- Refreshing the Node Metrics by calling `nodemetrics.py` using default settings.

---
Randal A. Koene, 20241026
