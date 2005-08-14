Some notes on using JamNNTPd with the hpt tosser from the Husky project:

 * The fidoconfig2jamnntpd.pl script found on the JamNNTPd homepage can be
   used to convert your area configuration to a jamnntpd.groups file.
 
 * If you want JamNNTPd to display the From address correctly, you need to
   run it with the -readorigin switch.

 * When hpt creates JAM messagebases, it does not create the lastread (*.jlr)
   file. JamNNTPd will not be able to open an area unless all four JAM files 
   are present. The checkgroups.pl script from the JamNNTPd homepage can be 
   used to create missing *.jlr files.

 * Do NOT pack your messagebase with "hpt pack". When hpt packs a messagebase,
   it will also renumber it. This will cause the article numbers in JamNNTPd
   to change which will confuse most newsreaders.

 * JamNNTPd can optionally create an echomail.jam files with the paths to 
   messagebases with messages to export. This file can not be used to tell 
   hpt what areas to scan, but it can be converted to an echotoss.log file 
   for hpt with the echomailjam2hpt.pl script.

   