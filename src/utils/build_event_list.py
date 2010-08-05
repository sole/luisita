#
# Build event list from Renoise's xrns song format
# to be used with luisita ;-)
# 
import sys
import re
import zipfile
from xml.dom.minidom import parseString

def main(argv=None):

	if argv is None:
		argv = sys.argv
	
	if argv is None or len(argv) < 3:
		print ""
		print "Usage: "
		print sys.argv[0], "/path/to/input.xrns /path/to/output.lua"
		print ""
		sys.exit(0)

	inputFile = argv[1]
	outputFile = argv[2]

	# Get & parse Song.xml
	songXMLData = None
	zsong = zipfile.ZipFile(inputFile, "r")
	for info in zsong.infolist():
		filename = info.filename
		if filename == 'Song.xml':
			songXMLData = zsong.read(filename)
			break
	zsong.close()

	if songXMLData is None:
		print "Song.xml could not be found, maybe ", inputFile, "wasn't actually a Renoise Song?"
		print "Anyway, I can't proceed beyond this point!"
		sys.exit(0)

	dom = parseString(songXMLData)
	
	# Final sanity check
	renoiseSong = getElementByTagName(dom, 'RenoiseSong')
	if int(renoiseSong.attributes['doc_version'].nodeValue) < 21:
		print "This song uses an older format (<21) that I can't parse."
		print "Please open it with Renoise, save it with a different name and use that one instead"
		sys.exit(0)
	

	# Process ~~~~~~~~~~~~

	initNoteMap()
	
	# GlobalSongData
	GlobalSongData = dom.getElementsByTagName("GlobalSongData")[0]
	song_bpm = getNodeInt(getElementByTagName(GlobalSongData, 'BeatsPerMin'))
	song_lpb = getNodeInt(getElementByTagName(GlobalSongData, 'LinesPerBeat'))
	print ( 'GlobalSongData--', 'bpm', song_bpm, 'lpb', song_lpb )

	# Tracks
	tracks = dom.getElementsByTagName("Tracks")[0]
	trackCount = 0
	track = tracks.firstChild
	"""while track:
		if track.nodeType == track.ELEMENT_NODE and track.tagName == "SequencerTrack":
			trackCount = trackCount + 1 # do not take into account non sequencer tracks such as master track, send tracks, etc
		track = track.nextSibling

	print(trackCount, 'tracks')
	
	track = tracks.firstChild"""
	songTrackColumns = []
	while track:
		if track.nodeType == track.ELEMENT_NODE and track.tagName == "SequencerTrack":
			numberOfNoteColumns = getNodeInt(getElementByTagName(track, 'NumberOfVisibleNoteColumns'))
			numberOfEffectColumns = getNodeInt(getElementByTagName(track, 'NumberOfVisibleEffectColumns'))
			print len(songTrackColumns), numberOfNoteColumns, 'note columns, ', numberOfEffectColumns, 'effect columns'
			songTrackColumns.append({'notes': numberOfNoteColumns, 'effects': numberOfEffectColumns})
			trackCount = trackCount + 1
		track = track.nextSibling

	print '-->', trackCount, 'tracks'
	print(songTrackColumns)
	
	# Patterns
	song_patterns= []
	patterns = dom.getElementsByTagName("Patterns")[0]
	numPatterns = int(getChildrenCount(patterns))
	print(numPatterns, 'patterns')
	
	pattern = patterns.firstChild
	
	while pattern:
		if pattern.nodeType == pattern.ELEMENT_NODE:
			print "--------------- Pattern %d ---------------- " % (len(song_patterns))
			numberOfLines = getNodeInt(getElementByTagName(pattern, "NumberOfLines"))
			obj_pattern = Pattern(numberOfLines, songTrackColumns)
			populatePattern(obj_pattern, pattern)
			song_patterns.append(obj_pattern)

		pattern = pattern.nextSibling
	
	# Order list
	song_order_list = []
	sequenceEntries = dom.getElementsByTagName("SequenceEntries")[0]
	sequenceEntry = getElementByTagName(sequenceEntries, "SequenceEntry")
	while sequenceEntry:
		if sequenceEntry.nodeType == sequenceEntry.ELEMENT_NODE:
			pattern = getElementByTagName(sequenceEntry, "Pattern")
			pattern_number = getNodeInt(pattern)
			song_order_list.append(pattern_number)
			print "sequ %d" % pattern_number
		sequenceEntry = sequenceEntry.nextSibling

	# Time values
	
	# And finally, really process the data in the patterns!!
	t = 0
	events = []
	secondsPerRow = 60.0 / (song_lpb * song_bpm);
	
	for i in range(0, len(song_order_list)):
		current_pattern_index = song_order_list[i]
		events.append(Event( type = EVENT_CHANGE_ORDER, timestamp = t, orderPosition = i, pattern = current_pattern_index ))
		
		current_pattern = song_patterns[current_pattern_index]
		events.append(Event( type = EVENT_CHANGE_PATTERN, timestamp = t, pattern = current_pattern_index ))

		for j in range(0, current_pattern.num_rows):
			events.append(Event( type = EVENT_CHANGE_ROW, timestamp = t, pattern = current_pattern_index, row = j ))
			
			for k in range(0, trackCount):
				cell = current_pattern.getCell(j, k)
				for column_index in range(0, len(cell.note_columns)):
					noteColumn = cell.note_columns[column_index]
					if noteColumn.note != NOTE_NULL and noteColumn.note != NOTE_OFF:
						events.append(Event( type = EVENT_NOTE_ON, timestamp = t, pattern = current_pattern_index, row = j, track = k, column = column_index, note = noteColumn.note, instrument = noteColumn.instrument, volume = noteColumn.volume ))
				
			
			t += secondsPerRow
		
	
	output = '--[[ AUTOMATICALLY GENERATED FROM %s - DO NOT MODIFY!!! ]]--\n' % (inputFile)
	output += "events_list = {}\n"
	
	for event in events:
		output += "table.insert(events_list, Event:new({timestamp= %f, type= %d, order_position= %d, pattern= %d, row= %d, track= %d, column= %d, note= %d, instrument= %d, volume= %f}))\n" % (event.timestamp, event.type, event.orderPosition, event.pattern, event.row, event.track, event.column, event.note, event.instrument, event.volume)

	f = open(outputFile, 'w')
	f.write(output)
	f.close()

## CO CO CO CONSTANTS

EVENT_NULL =          0
EVENT_NOTE_OFF =      1
EVENT_NOTE_ON =       2
EVENT_VOLUME = 3
EVENT_EFFECT = 4
EVENT_CHANGE_ORDER = 5
EVENT_CHANGE_PATTERN = 6
EVENT_CHANGE_ROW = 7
EVENT_SONG_END = 8

NOTE_NULL = -1
NOTE_OFF = -2
INSTRUMENT_NULL = -1
VOLUME_NULL = -1
TRACK_NULL = -1
COLUMN_NULL = -1
ORDER_NULL = -1
PATTERN_NULL = -1
ROW_NULL = -1
EFFECT_NULL = -1
EFFECT_VALUE_NULL = -1

############################

def populatePattern(pattern, xml):
	print 'populatePattern', pattern
	
	tracks = getElementByTagName(xml, "Tracks")
	trackCount = 0
	track = tracks.firstChild

	while track:
		if track.nodeType == track.ELEMENT_NODE and track.tagName == "PatternTrack":
			print "Track", trackCount
			lines = getElementByTagName(track, 'Lines')
			if lines != None:
				line = getElementByTagName(lines, 'Line')
				while line:
					if line.nodeType == line.ELEMENT_NODE:
						rowNumber = int(line.attributes['index'].nodeValue)
						print 'row', rowNumber
						
						cell = pattern.getCell(rowNumber, trackCount)
						
						noteColumns = getElementByTagName(line, 'NoteColumns')
						
						if noteColumns:
							noteColumn = getElementByTagName(noteColumns, 'NoteColumn')
							noteColumnIndex = 0
							
							while noteColumn:
								if noteColumn.nodeType == noteColumn.ELEMENT_NODE:
									noteCell = cell.getNoteColumn(noteColumnIndex) #cell.note_columns[noteColumnIndex]
									
									note = getElementByTagName(noteColumn, 'Note')
									if note:
										noteCell.note = noteMap[getNodeText(note)]
										
									instrument = getElementByTagName(noteColumn, 'Instrument')
									if instrument:
										noteCell.instrument = int(getNodeText(instrument), 16)
									
									volume = getElementByTagName(noteColumn, 'Volume')
									if volume:
										noteCell.volume = float(getNodeText(volume), 16) * 1.0 / 0x80 # Make it 0..1.0f
									
									noteColumnIndex = noteColumnIndex + 1
								noteColumn = noteColumn.nextSibling
						
					line = line.nextSibling
			else:
				print 'No rows'
			trackCount = trackCount + 1
		
		track = track.nextSibling

class Pattern:
	def __init__(self, num_rows, track_columns):
		self.rows = []
		self.num_rows = num_rows
		
		print('im a pattern', num_rows, track_columns)
		for i in range(0, num_rows):
			row = []
			k = 0
			
			for track in track_columns:
				
				cellNotes = []
				cellEffects = []
				
				for j in range(0, track['notes']):
					cellNotes.append(PatternNoteCell())
				
				for j in range(0, track['effects']):
					cellEffects.append(PatternEffectCell())
				#print 'track = %d | initialised cell with %d notes, %d effects' % (k, len(cellNotes), len(cellEffects))
				cell = PatternCell(note_columns = cellNotes, effect_columns = cellEffects)
				#print "Proof", cell
				row.append(cell)
				k = k + 1

			self.rows.append(row)
		print 'pattern num rows', len(self.rows), self
			
	def getCell(self, row, track):
		print "get cell, row = ", row, 'track=', track
		return self.rows[row][track]

class PatternCell:
	def __init__(self, note_columns, effect_columns):
		self.note_columns = note_columns
		self.effect_columns = effect_columns
		
	def getNoteColumn(self, index):
		print "getNoteColumn %d (len = %d)" % (index, len(self.note_columns))
		if len(self.note_columns) <= index:
			print 'trying to access %d but max is %d' % (index, len(self.note_columns))
			exit(0)
		else:
			return self.note_columns[index]
	
	def __str__(self):
		s = "PatternCell %d note columns %d effect columns" % ( len(self.note_columns), len(self.effect_columns))
		
		return s

class PatternNoteCell:
	def __init__(self, note = NOTE_NULL, instrument = INSTRUMENT_NULL, volume = VOLUME_NULL):
		self.note = note
		self.instrument = instrument
		self.volume = volume
		
class PatternEffectCell:
	def __init__(self, effect = EFFECT_NULL, effectValue = EFFECT_VALUE_NULL):
		self.effect = effect
		self.effectValue = effectValue

class Event:
	def __init__(self, type = EVENT_NULL, timestamp = 0, orderPosition = ORDER_NULL, pattern = PATTERN_NULL, row = ROW_NULL, note = NOTE_NULL, instrument =  INSTRUMENT_NULL, volume = VOLUME_NULL, track = TRACK_NULL, column = COLUMN_NULL, effect = EFFECT_NULL, effectValue = EFFECT_VALUE_NULL ):
		self.type = type
		self.timestamp = timestamp
		self.note = note
		self.instrument = instrument
		self.volume = volume
		self.track = track
		self.column = column
		self.effect = effect
		self.effectValue = effectValue
		self.row = row
		self.pattern = pattern
		self.orderPosition = orderPosition

def getText(nodelist):
	rc = ""
	for node in nodelist:
		if node.nodeType == node.TEXT_NODE:
			rc = rc + node.data
	return rc

def getNodeText(node):
	return node.childNodes[0].nodeValue

def getNodeInt(node):
	return int(getNodeText(node))

def getChildrenCount(node):
	if not node.hasChildNodes():
		return 0
	else:
		c = 0
		child = node.firstChild
		while child:
			if child.nodeType == child.ELEMENT_NODE:
				c = c + 1
			child = child.nextSibling
		return c

# Iterate through DIRECT children only
def getElementByTagName(node, tagName):
	if not node.hasChildNodes():
		return None
	else:
		child = node.firstChild
		while child:
			if child.nodeType == child.ELEMENT_NODE and child.tagName == tagName:
				return child
			child = child.nextSibling

	return None

noteMap = {}
def initNoteMap():
	notes = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
	for i in range(127):
		key = notes[i % 12]
		octave = i / 12
		
		if len(key) == 1:
			key = key + "-"
		
		key = key + ("%d" % octave)
		
		noteMap[key] = i
		
	noteMap["OFF"] = -2

# ----------------------------------------------------------------------------------

# And this is where the process is launched...
if __name__ == "__main__":
	main()
