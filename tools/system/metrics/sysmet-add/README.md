# sysmet-add - Add data to System Metrics

A metrics updating tools that provides a simple and straightforward way to add data to a metric
without having to know how and where the metrics are stored.

The command line tool can also be used as a CGI handler.

Command line example:

```
    sysmet-add -m calories -a 200
```

The command above means: Add 200 to the `calories` metric. By default the metric for today is
updated, although previous dates can be corrected by specifying the date.

---

The `sysmet-add-form.html` page offers a simple and direct web interface to `sysmet-add`.
