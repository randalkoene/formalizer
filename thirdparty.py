#!/usr/bin/python3
#
# Randal A. Koene, 20241111
#
# Check third party tools and prepare links to them as needed.

# Notes:
# - The third party tool md4c is available at https://github.com/mity/md4c
#   and can be built as specified in https://github.com/mity/md4c/wiki/Building-MD4C.

from sys import argv
from os.path import exists, basename
from os import symlink, remove

def found_tool(thirdparty:str)->bool:
	return exists(thirdparty)

def force_symlink(src, dst):
    try:
        symlink(src, dst)
    except FileExistsError:
        remove(dst)
        symlink(src, dst)

def make_links(exedir:str, cgidir:str, thirdparty:str):
	toolname = basename(thirdparty)
	print('Making symlinks for %s...' % toolname)
	force_symlink(thirdparty, exedir+'/'+toolname)
	force_symlink(thirdparty, cgidir+'/'+toolname)

def check_and_link(exedir:str, cgidir:str, args:list):
	for thirdparty in args:
		print('Checking third party tool %s...' % thirdparty)
		if not found_tool(thirdparty):
			print('WARNING: Missing third party tool %s!' % thirdparty)
		else:
			print('Found.')
			make_links(exedir, cgidir, thirdparty)

if __name__ == '__main__':
	exedir = argv[1]
	cgidir = argv[2]
	print('Local executables directory: '+exedir)
	print('CGI directory: '+cgidir)
	check_and_link(exedir, cgidir, argv[3:])
