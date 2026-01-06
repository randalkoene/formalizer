# replicate - Install and/or restore complete backed-up system environment

This aids the process of (re)installing a complete system environment,
containing specified programs, the Formalizer, and user data.

This assumes that the system environment was backed up as suppored by
the `core/fzbackup/backup-environment.py` script.

## Note 20251210

The replicate.py script was built step by step quite recently. Until it
stabilizes, running it once may fail to complete a system replication.
This is actually part of the stabilization process. Those failures will
be informative about how to improve the replicate.py sript.
To try again at the next step, just run the same replicate.py command
again, e.g:

./replicate.py /media/randalk/SSKSDDEXT4/randalk-ext4-moutable-20250603 /media/randalk/SSKSDDEXT4/mount-point

It's also very important to read the messages produced at the end of the
script, because they may still mention checks to perform and things that
need to be done.

--

Randal A. Koene, 20251002
