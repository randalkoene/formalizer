#!/usr/bin/python3
#
# Randal A. Koene, 20241026
#
# A script to convert the old idbook.html content to a JSON data file.
# This is then used to put the information into the database and to
# read and update it through a Formalizer page.

try:
    import cgitb; cgitb.enable()
except:
    pass
import sys, cgi, os
sys.stderr = sys.stdout
import re
import json
import traceback

idbookfile='/home/randalk/_private-info/doc/html/idbook-202006040905.html'

def get_table_row(content:str, idx:int)->tuple:
	row_idx = content.find("<tr>", idx)
	if row_idx < 0:
		return '', -1
	row_idx_end = content.find("</tr>", row_idx)
	if row_idx_end < 0:
		return '', -1
	return content[row_idx+4:row_idx_end], row_idx_end

def get_cell_content(content:str, idx:int)->tuple:
	cell_idx = content.find("<td", idx)
	if cell_idx < 0:
		return '', -1
	cell_idx_end = content.find("</td>", cell_idx)
	if cell_idx_end < 0:
		return '', -1
	cellcontent = content[cell_idx+3:cell_idx_end]
	data_idx = cellcontent.find("<p")
	if data_idx < 0:
		return '', -1
	data_idx = cellcontent.find(">", data_idx)
	if data_idx < 0:
		return '', -1
	data_idx_end = cellcontent.find("</p>", data_idx)
	if data_idx_end < 0:
		return '', -1
	cellcontent = cellcontent[data_idx+1:data_idx_end]
	return cellcontent.strip(), cell_idx_end

def get_data(cell1:str, cell2:str, cell3:str, cell4:str)->list:
	return [cell1, cell2, cell3, cell4]

def remove_html_tags(content:str)->str:
	raw_content = re.sub('[<][^>]*[>]','', content)
	return raw_content

def get_first_table(content:str)->list:
	table_data = []
	idx = content.find("<body")
	idx = content.find("<table", idx)
	idx_end = content.find("</table", idx)
	while idx > 0 and idx < idx_end:
		rowcontent, idx = get_table_row(content, idx)
		if idx >= 0:
			cell1_content, row_idx = get_cell_content(rowcontent, 0)
			if row_idx < 0:
				continue
			cell1_content = remove_html_tags(cell1_content)
			cell2_content, row_idx = get_cell_content(rowcontent, row_idx)
			if row_idx < 0:
				continue
			cell3_content, row_idx = get_cell_content(rowcontent, row_idx)
			if row_idx < 0:
				continue
			cell4_content, row_idx = get_cell_content(rowcontent, row_idx)
			if row_idx < 0:
				continue
			data = get_data(cell1_content, cell2_content, cell3_content, cell4_content)
			# Drop empty rows
			if cell1_content != '':
				table_data.append(data)
	return [ table_data, idx_end ]

def get_par_content(content:str, idx:int)->tuple:
	idx = content.find("<p", idx)
	if idx < 0:
		return None, idx
	idx = content.find(">", idx+2)
	if idx < 0:
		return None, idx
	idx_end = content.find("</p>", idx)
	if idx_end < 0:
		return None, idx_end
	parcontent = remove_html_tags(content[idx+1:idx_end]).strip()
	return parcontent, idx_end

def get_paragraphs(content:str, idx:int)->list:
	block_data = []
	idx = content.find("</p>\n<p ", idx)
	idx_end = content.find("<p><br/>\n<br/>", idx)
	while idx > 0 and idx_end > 0:
		blockcontent = content[idx+5:idx_end]
		data = []
		block_idx = 0
		while True:
			next_data, block_idx = get_par_content(blockcontent, block_idx)
			if next_data is None:
				break
			data.append(next_data)
		block_data.append(data)
		idx = content.find("</p>\n<p ", idx_end)
		if idx >=0:
			idx_end = content.find("<p><br/>\n<br/>", idx)
	return block_data

def get_second_table(content:str, idx:int)->list:
	table_data = []
	idx = content.find("<table", idx)
	idx_end = content.find("</table", idx)
	is_first_row = True
	while idx > 0 and idx < idx_end:
		rowcontent, idx = get_table_row(content, idx)
		if idx >= 0:
			cell1_content, row_idx = get_cell_content(rowcontent, 0)
			if row_idx < 0:
				continue
			cell2_content, row_idx = get_cell_content(rowcontent, row_idx)
			if row_idx < 0:
				continue
			cell3_content, row_idx = get_cell_content(rowcontent, row_idx)
			if row_idx < 0:
				continue
			cell4_content, row_idx = get_cell_content(rowcontent, row_idx)
			if row_idx < 0:
				continue
			cell5_content, row_idx = get_cell_content(rowcontent, row_idx)
			data = [ cell1_content, cell3_content, cell4_content, cell2_content, cell5_content ]
			# Drop the first row, because it's the table header
			if is_first_row:
				is_first_row = False
			else:
				table_data.append(data)
	return [ table_data, idx_end ]

def reduce_to_four(datalist:list)->list:
	reduced_list = []
	for linedata in datalist:
		if len(linedata)>4:
			for i in range(4,len(linedata)):
				linedata[3] += '<br>\n' + linedata[i]
		elif len(linedata)<4:
			for i in range(len(linedata), 4):
				linedata.append('')
		reduced_list.append(linedata[0:4])
	return reduced_list

def parse_idbook(content:str)->list:
	part1, idx = get_first_table(content)
	part2 = reduce_to_four(get_paragraphs(content, idx))
	part3, idx = get_second_table(content, idx)
	part3 = reduce_to_four(part3)
	return part1 + part2 + part3

def test_display_of_data(data:list):
	for line in data:
		print('<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>' % (line[0], line[1], line[2], line[3]))

if __name__ == '__main__':
	is_cgi = True
	if is_cgi:
		print("Content-type: text/plain\n\n")
		with open(idbookfile, 'r') as f:
			idbookcontent = f.read()
		data = parse_idbook(idbookcontent)
		data.sort(key=lambda x: x[0].lower())

		print(json.dumps(data))

	else:
		with open(idbookfile, 'r') as f:
			idbookcontent = f.read()
		data = parse_idbook(idbookcontent)
		data.sort(key=lambda x: x[0].lower())
		print('<html><body>Number of rows: %s<table><tbody>' % str(len(data)))
		print('<style>table, th, td { border: 1px solid; }</style>')
		test_display_of_data(data)
		print('</tbody></table></body></html>')
