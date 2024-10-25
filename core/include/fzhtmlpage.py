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
SUBMIT_ON_INPUT=' onchange="formUpdate(this)"' # NOTE: For this to work the value may need to change.
TEST_ON_INPUT=' onchange="testUpdate(this)"'

HTML_STD_TOP='''<!DOCTYPE html>
<html style="overflow-x:scroll;">
<html>

'''
# BEWARE: *** For some reason, the POST method works with daywiz.py but not with
#             consumed.py (where form data is not properly received, although
#             it is sent in 'quantity', as if the form is posted before the
#             Javascript function can update par_changed), but consumed.py does
#             work with the GET method.
HTML_STD_BODY='''<body>
<form id="mainForm" action="/cgi-bin/%s" method="%s">
<input type="hidden" name="cmd" value="update">
<input type="hidden" id="par_changed" name="par_changed" value="unknown">
<input type="hidden" id="par_newval" name="par_newval" value="">
'''
HTML_STD_TAIL='''</form><hr>

<p>[<a href="/index.html">fz: Top</a>]</p>
'''
# NOTE: The functions below require and id, not just a name.
# BEWARE: *** Right now this seems to work for daywiz.py but not for consumed.py.
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
function testUpdate(test_ref) {
    console.log('TESTING');
    return false;
}
window.addEventListener('keydown', (event) => {
  if (event.key === 'Enter') {
    event.preventDefault();
  }
});
</script>
</body>

</html>
'''
class fz_html_standard:
    def __init__(self, cgibin: str, method='post'):
        self.top_content = HTML_STD_TOP
        self.head_content = '<meta charset="utf-8" />\n'
        self.body_content = HTML_STD_BODY % (cgibin, method)
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

class fz_html_tooltip:
    def __init__(self):
        self.head_content = '<link rel="stylesheet" href="/tooltip.css">\n'

    def generate_html_head(self) ->str:
        return self.head_content

    def generate_html_body(self) ->str:
        return ''

    def generate_html_tail(self) ->str:
        return ''

HTML_CLOCK_TAIL='''<script type="text/javascript" src="/clock.js"></script>
'''
class fz_html_clock:
    def __init__(self):
        self.head_content = '<link rel="stylesheet" href="/clock.css">\n'
        self.body_content = ''
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

# Note that list_of_str_tuples_generator can simply generate or return a list
# of strings if bodyline_template contains only one place holder.
class fz_html_multilinetable:
    def __init__(self, head_template: str, bodyframe_template: str, bodyline_template: str, list_of_str_tuples_generator):
        self.head_template = head_template
        self.bodyframe_template = bodyframe_template
        self.bodyline_template = bodyline_template
        self.list_of_str_tuples_generator = list_of_str_tuples_generator
        self.lines_html = ''

    def generate_html_head(self) ->str:
        return self.head_template

    def generate_html_body(self) ->str:
        self.lines_html = ''
        for str_tuple in self.list_of_str_tuples_generator():
            self.lines_html += self.bodyline_template % str_tuple
        return self.bodyframe_template % self.lines_html

class fz_html_multicomponent:
    def __init__(self, head_template: str, body_template: str):
        self.head_template = head_template
        self.body_template = body_template
        self.components = {}

    def generate_html_head(self) ->str:
        return self.head_template

    def generate_data_tuple(self) ->tuple:
        return tuple( [ comp.generate_html_body() for comp in self.components.values() ] )

    def generate_html_body(self) ->str:
        return self.body_template % self.generate_data_tuple()

# Use this to deliver a number of horizontal table components and auto-generate
# the necessary table frame cells, as well as the components dictionary for
# fz_html_multicomponent. E.g. see how this is done in consumed.py.
class fz_html_horizontal_component_cells:
    def __init__(self, tables_frame_begin:str, tables_frame_end:str, headers_list:list, components_dict:dict):
        self.headers_list = headers_list
        self.components_dict = components_dict
        self.tables_frame_begin = tables_frame_begin
        self.tables_frame_end = tables_frame_end
    def get_components(self)->dict:
        return self.components_dict
    def get_headers(self)->str:
        headers_str = ''
        for header in self.headers_list:
            headers_str += '<th>'+header+'</th>'
        return headers_str
    def get_tables_frame(self)->str:
        frame_str = self.tables_frame_begin % self.get_headers()
        for i in range(len(self.components_dict)):
            frame_str += '<td>%s</td>\n'
        frame_str += self.tables_frame_end
        return frame_str

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

ERROR_FRAME='''<p>
Error: <b>%s</b>
</p>
'''
class fz_errorpage:
    def __init__(self, error_message: str):
        self.error_message = error_message

    def generate_html_head(self) ->str:
        return ''

    def generate_html_body(self) ->str:
        return ERROR_FRAME % str(self.error_message)

    def generate_html_tail(self) ->str:
        return ''
