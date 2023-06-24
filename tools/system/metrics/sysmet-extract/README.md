# sysmet-extract -- Extract System Metrics

Deduce System Metrics data from information in Log and Graph.

`fzlogmap -D 10 -r -o STDOUT -q -f ~/src/formalizer/tools/system/metrics/sysmet-extract/categories_a2c.json`
`fzlogmap -D 7 -r -o STDOUT -q -C -f ~/src/formalizer/tools/system/metrics/sysmet-extract/categories_a2c.json`
`fzlogmap -D 7 -r -o STDOUT -q -f ~/src/formalizer/tools/system/metrics/sysmet-extract/categories_work.json`
`fzlogmap -D 10 -r -o STDOUT -q -f ~/src/formalizer/tools/system/metrics/sysmet-extract/categories_hourly.json`

---

## Developing Formalizer 2.x versions of metrics collection

Collecting data for the daily time tracking in the Formalizer 1.x was carried out by scripts such as `self-eval-metrics.sh` that use the `-zMAP` option of `dil2al`. The coresponding functions are defined in `dil2al/tlfiler.cc`, especially the `System_self_evaluation()` function.

That function, and the ones it calls can be used as instructional guides for the development of new ones, while the most important step is to understand what each metric stands for. With that understanding, a literal re-implementation is not necessary. The best result might be one where the intended and desired metrics data is obtained from a fresh re-evaluation of the data that is in the Log and Graph, as well as additional data stored in spreadsheets.

It is fine to start simple, for example, by commencing with a single metric, e.g. daily sleep hours.

## Categories specification files

Here we describe the main Category grouping specification files provided, their purpose, and their relationship to similar specifications in the Formalizer 1.x environment.

1. `categories_a2c.json`: Also known as "Intentions Performance" through the Formalizer dashboard buttons. First defined around 2019.
2. `categories_hourly.json`: Based on Formalizer 1.x task metrics categories. In use up to about Jan. 2021.
3. `categories_work.json`: Also known as "Work Performance" through the Formalizer dashboard buttons.
4. `categories_main2023.json`: A quick metric tracking the balance of effort on Voxa, CCF and other life Values. Introduced in 2023.

### categories_a2c.json - Activities to Categories

The specifications in `categories_a2c.json` provide categories as used during daily Morning System metrics updates. They correspond to the categories specified in `formalizer.self-evaluation-categories.rc` used with the Formalizer 1.x environment up to Jan. 2021. Description copied from that file:

> This set of categories is intended to produce evaluation data
> that is directly usable for Formalizer daily self-evaluation.
> Randal A. Koene 20190127
>
> Update 20200420: Created the BUILDSYSTEM category and moved a
> number of DIL files there, as explained in TL#202004202027.1.
> This new category is now also included in the Time Tracking
> variable "day Intentions hours", due to an update in the
> dil2al function `tlfilter.cc:System_self_evaluation()`.
>
> Update 20200418: Moved `dil2al.html`, `formalizer.html` and
> `computer-environment.html` from HOBBIES to SYSTEM. This is explained in
> TL#202004182210.1.
>
> Update 20200326: Added 24 DIL Files explicitly that had not
> been mentioned in this `.rc` file (found with `dil2al-containsallDILfiles.sh`).
> A backup copy of the previous version of this `.rc` file was
> made in `formalizer.self-evaluation-categories.rc-v20200326` just
> in case there was a reason why not all DIL files were mentioned
> here (even some that existed when this was first created).
>
> Some information about how this file works:
> a) Lines that begin with `#` are comment lines and are ignored
> b) Blank (white space) lines are ignored
> c) Comments or notes following DIL ID numbers are ignored
> d) Lines beginning with `<category>`: start a new category definition
> e) If the `<category>` is folled by `=<value>` (e.g. `SLEEP=0xC5:`) then the value is used in `-zMAP`
> f) Other lines define members of a category
> g) Category members can be specific DIL IDs or DIL file names
>
> During processing, the precedence order is:
>
> 1. `@label=x.y@` search is carried out first unless in `-zMAP` mode.
>    It is in addition to following metrics steps, not instead of.
> 2. `@CATEGORY:<category>@` when specified in Task Chunk text.
> 3. Category by Task ID.
> 4. Category by DIL file.
> 5. The "other" category is otherwise applied.
>    If other: is explicitly specified then it must appear LAST.

The categories are:

> Time spent sleeping or napping `CMYK=11000010=0xC2` (was `CMYK=10000100=0x84`) (turquoise-cyan)
> (This is reported in Time Tracking.)  
`SLEEP=0xC2:`

> IPA/B I.P. Active and Build and closely related (historical ones included) `CMYK=11001100=0xCC` (green)
> (This is reported in Time Tracking, for the whole day and for the morning hours, and as part of "day Intentions hours".)  
`IPAB=0xCC:`

> Day Intentions not already included in IPAB or SYSTEMBUILD `CMYK=10001001=0x89` (grass green)
> Note that a bunch of rarely used `oop-*.html` files have not been categorized yet.
> (This is reported in Time Tracking as part of "day Intentions hours".)  
`DAYINT=0x89:`

> System Build `CMYK=01000101=0x45` (mid-green)
> Until 2020-04-20 these were in SYSTEM. See TL#202004202000.1.
> (This is reported in Time Tracking as part of "day Intentions hours".)  
`BUILDSYSTEM=0x45:`

> System Infrastructure `CMYK=11110001=0xF1` (blue)
> Note that some tasks in `self-improvement.html` probably better fit in different categories.
> Note that about 20 percent of tasks in `networking.html` probably should be in BUILDSYSTEM or DAYINT.  
`SYSTEM=0xF1:`

> Time spent on hobbies, interests, certain productive procrastination `CMYK=00001100=0x0C` (yellow)
> Note that some `mobile-living.html` tasks better fit in different categories.  
`HOBBIES=0x0C:`

> Time spent making or having meals `CMYK=00011110=0x1E` (brick)  
`MEALS=0x1E:`

> Time spent on solo entertainment `CMYK=00111100=0x3C` (bright red / neon red)
> (This is reported in Time Tracking.)  
`SOLO=0x3C:`

> Socializing `CMYK=00111000=0x38` (pink)
> Note that `leisure.html` contains a lot of festival tasks, some of which could be HOBBIES or TRAVEL.
> (This is reported in Time Tracking.)  
`SOCIAL=0x38:`

> Community activities and chores `CMYK=01010001=0x51` (violet)
> Note that there might ultimately be better categories for `family.html` tasks.  
`CHORES=0x51:`

> Travel and travel related activities `CMYK=00101100=0x2C` (orange)  
`TRAVEL=0x2C:`

> Well-being and health related activities `CMYK=01001000=0x48` (moon green)
> (This is reported in Time Tracking.)  
`WELLBEING=0x48:`

> When specified explicitly, the "other" category must be listed last `CMYK=01010111=0x97` (dark gray / black)  
`other=0x97:`

### categories_hourly.json - Ongoing Activities Tracking

The specifications in `categories_hourly.json` provide categories as used regularly during the day to track the ongoing distribution of effort. They correspond to the categories specified in `formalizer.task-metrics-categories.rc` used with the Formalizer 1.x environment up to Jan. 2021. Description copied from that file:

> This `.rc` metrics categories files is used to generate regular
> relative effort visualizatons, such as with the script
> combination `dil2al-taskdata-hourly.sh` + `taskdataa2chourly.m`.
>
> There are some placements that might deserve re-evaluation,
> for example:
>   `profiles.html`
>   `system.html`
>   `neuromem.html` (because it's a symbolic link)
>   `rtt.html`
>   `formalizer.html`
>   `capital.html`
>   `oop.html`
>   `oop-change.html`
>   `travel.html`
>   `health.html`
>   `community.html`
>   `family.html`

The categories are:

>  
> `ACTIONS:`

>  
> `INFRASTRUCTURE:`

>  
> `SLEEP:`

>  
> `other:`

### categories_work.json - Tracking Types of Work

These specifications were developed with Formalizer 2.x. The documentation below applies to the corresponding JSON file.

(Documentation to be added here.)

### categories_main2023.json - 2023 Three Main Groups

These specifications were developed with Formalizer 2.x. The documentation below applies to the corresponding JSON file.

A quick metric tracking the balance of effort on Voxa, CCF and other life Values.



---
Randal A. Koene, 2020, 2021, 2023
