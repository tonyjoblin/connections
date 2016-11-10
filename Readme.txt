# connections

## About

A c++ program to parse a timetable file in xml and produce
connections. The timetable is read from a file specified on
the command line and the connections are written to
standard ouput.

## Details

The program uses boost property tree library for xml parsing.
This loads the entire xml document into memory at startup.
This worked fine except the laptop I was using had limited
memory and so it couldn't process the entire xml file.

For later work I switched to using libxml2 pull reader.
