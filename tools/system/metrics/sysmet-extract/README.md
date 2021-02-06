# sysmet-extract -- Extract System Metrics

Deduce System Metrics data from information in Log and Graph.

---

## Developing Formalizer 2.x versions of metrics collection

Collecting data for the daily time tracking in the Formalizer 1.x was carried out by scripts such as `self-eval-metrics.sh` that use the `-zMAP` option of `dil2al`. The coresponding functions are defined in `dil2al/tlfiler.cc`, especially the `System_self_evaluation()` function.

That function, and the ones it calls can be used as instructional guides for the development of new ones, while the most important step is to understand what each metric stands for. With that understanding, a literal re-implementation is not necessary. The best result might be one where the intended and desired metrics data is obtained from a fresh re-evaluation of the data that is in the Log and Graph, as well as additional data stored in spreadsheets.

It is fine to start simple, for example, by commencing with a single metric, e.g. daily sleep hours.
