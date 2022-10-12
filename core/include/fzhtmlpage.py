# Copyright 2022 Randal A. Koene
# License TBD

"""
Import this module to provide general base classes that facilitate
dynamic HTML page generation. For example, see how this is used
by `daywiz.py`.

Versioning is based on https://semver.org/. See coreversion.hpp for more.
"""

from datetime import datetime

SUBMIT_ON_CHANGE=' onchange="chkbxUpdate(this)"'
SUBMIT_ON_INPUT=' onchange="formUpdate(this)"'

HTML_STD_TOP='''<!DOCTYPE html>
<html>

'''
HTML_STD_BODY='''<body>
<form id="mainForm" action="/cgi-bin/%s" method="post">
<input type="hidden" name="cmd" value="update">
<input type="hidden" id="par_changed" name="par_changed" value="unknown">
<input type="hidden" id="par_newval" name="par_newval" value="">
'''
HTML_STD_TAIL='''</form><hr>

<p>[<a href="/index.html">fz: Top</a>]</p>
'''
HTML_STD_BOTTOM='''<script>
function chkbxUpdate(chkbx_ref) {
	var id = chkbx_ref.id;
	var val = chkbx_ref.checked;
	console.log(`${id} ${val}`);
	var par_changed = document.getElementById("par_changed");
	par_changed.value = id;
	var par_newval = document.getElementById("par_newval");
	par_newval.value = val;
	document.getElementById("mainForm").submit();
}
function formUpdate(input_ref) {
	var id = input_ref.id;
	var val = input_ref.value;
	console.log(`${id} ${val}`);
	var par_changed = document.getElementById("par_changed");
	par_changed.value = id;
	var par_newval = document.getElementById("par_newval");
	par_newval.value = val;
    document.getElementById("mainForm").submit();
}
</script>
</body>

</html>
'''
class fz_html_standard:
	def __init__(self, cgibin: str, ):
		self.top_content = HTML_STD_TOP
		self.head_content = '<meta charset="utf-8" />\n'
		self.body_content = HTML_STD_BODY % cgibin
		self.tail_content = HTML_STD_TAIL
		self.bottom_content = HTML_STD_BOTTOM

	def generate_html_top(self) ->str:
		return self.top_content

	def generate_html_head(self) ->str:
		return self.head_content

	def generate_html_body(self) ->str:
		return self.body_content

	def generate_html_tail(self) ->str:
		return self.tail_content

	def generate_html_bottom(self) ->str:
		return self.bottom_content

class fz_html_icon:
	def __init__(self):
		self.icon = '/favicon-32x32.png'

	def generate_html_head(self) ->str:
		return '<link rel="icon" href="%s">\n' % self.icon

class fz_html_style:
	def __init__(self, style_list: list):
		self.style_list = style_list

	def generate_html_head(self) ->str:
		head_str = ''
		for stylename in self.style_list:
			head_str += '<link rel="stylesheet" href="/%s.css">\n' % stylename
		return head_str

class fz_html_uistate:
	def __init__(self):
		self.head_content = '<link rel="stylesheet" href="/fzuistate.css">\n'
		self.tail_content = '<script type="text/javascript" src="/fzuistate.js"></script>\n'

	def generate_html_head(self) ->str:
		return self.head_content

	def generate_html_body(self) ->str:
		return ''

	def generate_html_tail(self) ->str:
		return self.tail_content

HTML_CLOCK_TAIL='''<script type="text/javascript" src="/clock.js"></script>
<script>
    var clock = new floatClock('clock');
</script>
'''
class fz_html_clock:
	def __init__(self):
		self.head_content = '<link rel="stylesheet" href="/clock.css">\n'
		self.body_content = '<button id="clock" class="button button2">_____</button>\n'
		self.tail_content = HTML_CLOCK_TAIL

	def generate_html_head(self) ->str:
		return self.head_content

	def generate_html_body(self) ->str:
		return self.body_content

	def generate_html_tail(self) ->str:
		return self.tail_content

class fz_html_title:
	def __init__(self, title: str):
		self.title = title

	def generate_html_head(self) ->str:
		return '<title>fz: %s</title>\n' % self.title

	def generate_html_body(self) ->str:
		return '<h1>fz: %s</h1>' % self.title

DATEPICKER_FRAME='''<input type="date" id="date" name="date" min="1990-01-01" value="%s" %s>
'''
class fz_html_datepicker:
	def __init__(self, day: datetime):
		self.day = day

	def generate_html_body(self) ->str:
		return DATEPICKER_FRAME % ( self.day.strftime('%Y-%m-%d'), SUBMIT_ON_INPUT )

class fz_htmlpage:
	def __init__(self):
		self.head_list = []
		self.body_list = []
		self.tail_list = []

	def generate_html_head(self) ->str:
		head_str = self.html_std.generate_html_top() + '<head>\n'
		for head_element in self.head_list:
			head_str += head_element.generate_html_head()
		head_str += '</head>\n'
		return head_str

	def generate_html_body(self) ->str:
		body_str = ''
		for body_element in self.body_list:
			body_str += body_element.generate_html_body()
		return body_str

	def generate_html_tail(self) ->str:
		tail_str = ''
		for tail_element in self.tail_list:
			tail_str += tail_element.generate_html_tail()
		tail_str += self.html_std.generate_html_bottom()
		return tail_str

	def generate_html(self) ->str:
		page_str = self.generate_html_head()
		page_str += self.generate_html_body()
		page_str += self.generate_html_tail()
		return page_str
