# Installation Notes

## Interference by browser extensions

Browser extensions that examine or filter http calls may interfere with or
slow down Formalizer web operation. If certain pages appear to open very
slowly, add the Formalizer server address(es) to the white list of the
extension involved (e.g. uBlock Origin Lite). For example:

```
localhost
127.0.0.1
```

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
