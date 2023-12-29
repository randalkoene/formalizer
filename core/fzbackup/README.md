# fzbackup - Formalizer core utility to back up the Formalizer environment

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

--
Randal A. Koene 2023
