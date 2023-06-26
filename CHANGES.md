20230626
- Added fzdaily.sh call to fzbackup-mirror-to-github.sh.

20230624

- Updating fzbackup-mirror-to-github.sh script to combine mirroring
  to github and copying of DayWiz JSON data in a single command.
- Created a safety backup ~/src/formalizer-20230624.tar.gz and
  pushed updated git commit on branch dailyuseful. It has been
  a long time since the last push, and the safety backup provides
  some assurance that things don't get messed up. In fact, it
  looks like no push had ever been done from harmonia, so the
  credentials for that were added.
- Updated wiztable.py to include instructions on running
  fzbackup-mirror-to-github.sh.

20230614

- Added "recent_days" line to fzlogtime tail template.
- Added recent days links list in render.cpp of fzlogtime.
