#!/usr/bin/env python3
# dayreview.py
# Copyright 2024 Randal A. Koene
# License TBD

print('Please answer the following questions about the preceding day.\n')

starttimestr = input('What time did the day begin ("HHMM", e.g. "0920")? : ')
startmin = int(starttimestr[-2:])
starthr = int(starttimestr[:-2])

endtimestr =   input('What time did the day end ("HHMM", e.g. "0030")?   : ')
endmin = int(endtimestr[-2:])
endhr = int(endtimestr[:-2])

if endhr < 12:
	endhr = endhr + 24

starthours = float(starthr) + (float(startmin)/60.0)
endhours = float(endhr) + (float(endmin)/60.0)

wakinghours = endhours - starthours

print('Number of waking hours in previous day: %.2f' % wakinghours)

INSTRUCTIONS = '''
Please review the Log Chunks in the previous day. Take note of
Chunks of the following kind:

  w - Work
  s - Self-Work
  S - System or self-care
  n - Nap

For each Chunk, you will be asked to indicate the hours or minutes
that were logged (e.g. "3.4" hours or "97" minutes). Then you will
be asked to indicate the type (w, s, S, n).

If you give an empty response you will be asked if you wish to redo
the current entry or if you have completed entries.
'''

print(INSTRUCTIONS)

def get_data(inputstr:str)->tuple:
	while True:
		redo=False
		done=False
		datastr = input(inputstr)
		if not datastr:
			while not redo and not done:
				redo_or_done = input('Do you wish to [r]edo, or are you [d]one? ')
				if not redo_or_done:
					redo=True
				else:
					if redo_or_done[0]=='r':
						redo=True
					elif redo_or_done[0]=='d':
						done=True
					else:
						redo=True
		if done:
			return (False, '')
		if not redo:
			return (True, datastr)

chunkdata = []
while True:

	validdata, data = get_data('Hours or minutes in Log Chunk: ')
	if not validdata:
		break
	if '.' in data:
		try:
			chunkhours = float(data)
		except:
			print('Discarding entry (%s). Not interpretable as hours or minuntes.' % data)
			continue
	else:
		try:
			chunkhours = int(data)/60.0
		except:
			print('Discarding entry (%s). Not interpretable as hours or minuntes.' % data)
			continue
	if chunkhours == 0.0:
		print('Discarding entry (%s), because interval hours are zero.' % str(data))
		continue

	validdata, data = get_data('Type of Chunk ([w]ork, [s]elf-work, [S]ystem/care, [n]ap): ')
	if not validdata:
		continue
	if data[0] not in 'wsSn':
		print('Discarding entry, because "%s" is an unknown type.' % str(data[0]))
		continue
	chunkdata.append( (chunkhours, data[0]) )
	print('%s Chunks entered.\n' % str(len(chunkdata)))

def sum_of_type(typeid:str)->float:
	hours = 0.0
	for i in range(len(chunkdata)):
		if chunkdata[i][1] == typeid[0]:
			hours += chunkdata[i][0]
	return hours

selfworkhours = sum_of_type('s')
workhours = sum_of_type('w')
systemhours = sum_of_type('S')
naphours = sum_of_type('n')

otherhours = (wakinghours - naphours) - (selfworkhours + workhours + systemhours)

print('Self-work hours  : %.2f' % selfworkhours)
print('Work hours       : %.2f' % workhours)
print('System/care hours: %.2f' % systemhours)
#print('Nap hours        : %.2f' % selfworkhours)
print('\nOther hours      : %.2f' % otherhours)

print('\nFormatted for addition to Log:\n\n')

OUTPUTTEMPLATE='The waking day yesterday was from %s to %s, containing about %.2f waking (non-nap) hours. I did %.2f hours of self-work and %.2f hours of work. I did %.2f hours of System and self-care. Other therefore took %.2f hours.'

print(OUTPUTTEMPLATE % (
		str(starttimestr),
		str(endtimestr),
		(wakinghours - naphours),
		selfworkhours,
		workhours,
		systemhours,
		otherhours,))
