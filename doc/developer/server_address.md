# Finding the server API address

There are numerous reasons why the server API address can be needed, both for
back-end and front-end components, typically to make Formalizer server
API requests. For example:

- The server address is needed to to serve up permitted local filesystem files
  (see `fzserverpq -h`).
- The server address is needed for CGI forwarding (see `fzserverpq -h`).
- It is needed to run FZ serialized data requests (see `fzserverpq -h`).
- It is needed for HTTP requests to the special server `/fz/` path operations
  (see `fzserverpq -h`).

During launch of the `fzserverpq`, the port number is obtained from the
`port_number` parameter in `~/.formalizer/config/fzserverpq/config.json`.

## Finding server address from the back-end

By default, when the `fzserverpq` process is started, it stores its
`<ip-address:port>` (without line-end) in a file at:
	`~/.formalizer/server_address`

The same file is also available through a symlink in `/usr/lib/cgi-bin`,
so that CGI scripts can use it.

## Finding server address from a C++ Formalizer component

These components can access `fzserverpq` data and information very efficiently
using Formalizer core library classes and functions. There are two steps
involved:

1. Gain access to server data through a library member function such as the
   `graph_mem_managers::get_Graph()` function in `Graphtypes.hpp`. For example,
   see how this is done in source code of the utility `fzlogdata`.
2. Then call `Graph::get_server_IPaddr()`, or `Graph::get_server_port()`, or
   `Graph::get_server_port_str()`, or `Graph::get_server_full_address()`.

## Finding server address from the front-end (web browser)


...one solution is to provide a CGI script that forwards API calls via fzgraph -C. This is already available via the `fzgraph-cgi.py` "generic" call with argument "action=generic",
followed by the argument-value pairs for command line arguments to fzgraph. The output can be specified to be "minimal" or an HTML page. It can be useful to have an even simpler
CGI option such as `fzapicall` that grabs a single argument string and uses `fzgraph -C` to send it and return the output as `Content-type:text/plain`.

### Option 1: Use API forwarding via provided CGI scripts

There are two Formalizer CGI scripts that can easily forward API calls:

- `fzgraph-cgi.py?action=generic`: This will forward any `fzgraph` call with
  `argument=value` pairs. If the special argument `minimal=on` is provided
  then the result is returned as minimal textual output (but as `text/html`).
- `fzapicall.py?apicall=URI-encoded-string`: This will decode the URI encoded
  string, forward it to the server via `fzgraph -C`, and will return the
  standard output as `Content-type:text/plain`.

### Option 2: Use FZ standard Python modules available to CGI scripts

The CGI-available `tcpclient` and `Graphaccess` modules are very useful here.
The `tcpclient module` has the function `get_server_address()`, and the
`Graphaccess` module has useful functions such as:

- `client_socket_request()`
- `serial_API_request()`

To see how these are used, for example, look at their use in `fzupdate-cgi.py`,
where these calls are used to manipulate NNLs.

### Option 3: Auto-generate server address into HTML element data

When building an HTML page generator where the resulting page should have JS access
to the server API address, do the following:

1. Use the C++ back-end method to obtain the server API address.
2. Include in the generated HTML either a) a bit of `<script>` where a
   `const` is set with that address, or b) an HTML element where a
   data parameter is added and given the server address (directly or
   by letting a JS script set it).

## Auto-generated server address in Log / Node content

A special code is provided, so that HTML output by `fzgraphhtml` and by
`fzloghtml` automatically converts those tags to server addresses.

You can include the tag `@FZSERVER@` at the start of a URL in an HREF when
editing Log or Node content. For example, `<a href="@FZSERVER@/doc/">` will
be converted when the Log or Node content is displayed in a generated
HTML page.

Doing this is simplified somewhat by using copy-templates from the hover-over
drop-down boxes typically provided on pages with entry text areas.

## Good practice for auto-generated HTML pages

A necessary server address can be baked into generated HTML pages. A standard
way to do this is to use HTML templates, and to use the `render_environment`
class to fill in template variables, e.g. `{{ fzserver-address }}`.

## Alternatives to direct TCP calls to the server

Instead of finding the server TCP address to make requests directly, calls can
be made via Foramlizer utility components:

- Using the `fzgraph -C <api-string>` utility call (see `fzgraph -h`).
- Using the `fzquerypq -Z <serialized-request>` utility call (see `fzquerypq -h`).

This is available to both back-end and front-end, because these utilities are
available both ways.

## Notes

This topic of finding the server address, for example to carry out API requests,
is quite different than needing to run a permitted CGI script or program
as the server user. For that topic, look up options such as
`serial_API_request(f'CGIbg_run_as_user())`.

---
Randal A. Koene, 2026
