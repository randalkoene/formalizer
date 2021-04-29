# fz: Timer

A general purpose web interface to a timer. The timer can be controlled directly through that interface or
can be controlled remotely via socket commands.

This timer interface can be used as a stand-alone tool, but it was primarily developed as a component
to be integrated with other System support tools.

**Note**: It is called `timer` and not `fztimer`, because it is one of many possible visual interface
implementations for a timer, not a fundamental service. Something that would qualify for the `fztimer`
name would probably operate more in the form of a background process or daemon.
