
                               JamNNTPd 0.6

                             by Johan Billing

                            (billing@df.lth.se)

                                2003-12-20

1. Introduction
===============
JamNNTPd is an attempt to merge dying fidonet technology with modern Usenet
newsreaders. Basically, JamNNTPd is NNTP server that allows newsreaders to
access a JAM messagebase. (If you didn't understand a word of this, you
probably don't want to use JamNNTPd anyway).

JamNNTPd can be used with both Linux and Windows. An executable binary is
only suppplied for Windows, Linux users need to compile JamNNTPd themselves.

2. Copyright
============
The copyright of JamNNTPd belongs to Johan Billing. Permission to use, copy
and distribute JamNNTPd is granted provided that this copyright notice is
included. Permission to modify JamNNTPd is granted. Distributing modified
versions of JamNNTPd is allowed provided that the documentation clearly
states that it is a modified version. Parts of JamNNTPd may be freely used
in other projects as long as credit is given.

JamNNTPd uses JAMLIB for reading and modifying JAM messagebases. JAMLIB is
copyright 1996 Björn Stenberg and is released under the GNU Lesser General
Public License (see included file src/jamlib/LICENSE).

3. Security
===========
I cannot guarantee that there are no security leaks in JamNNTPd. If you
decide to use it, you do it on your own risk. If you use JamNNTPd under
Linux, you should avoid running it with root privileges.

4. Using JamNNTPd
=================

4.1 Command-line options
------------------------
Usage: jamnntpd [-debug] [-noecholog] [-nostripre] [-notearline]
                [-noreplyaddr] [-smartquote] [-noencode] [-keepsoftcr]
                [-notzutc] [-p <port>] [-m <maxconn>] [-def_flowed on/off]
                [-def_showto on/off] [-origin <origin>] [-g <groupsfile>]
                [-a <allowfile>] [-u <usersfile>] [-x <xlatfile>]
                [-l <logfile>]

 -debug

   If this option is used, JamNNTPd will print att sent and received text
   to the console window. Useful for testing.

 -noecholog

   Disables echoing of log messages to the console window.

 -nostripre

   JamNNTPd normally strips "Re:" from subject lines of followups. Use
   this option if you want to retain the "Re:".

 -notearline

   JamNNTPd normally puts the information from the X-Newsreader or User-Agent
   header field in the tearline of posted messages. This option disables this
   behaviour and leaves the tearline blank.

 -noreplyaddr

   JamNNTPd normally adds a REPLYADDR kludge with the e-mail address of the
   sender in posted messages. Use this option if you don't want REPLYADDR
   kludges. See also see section 6.4 below.

 -smartquote

   The quoting style of most newsreaders is different from traditional fidonet
   software. If you enable this option, JamNNTPd will try to change any quoted
   lines to fidonet style. This means that it will try to insert the initials
   of the person you reply to before the '>' character and also that it will try
   to compound multiple generations of quotes, i.e. "AA> BB>" will be changed
   into "BB>>".

   Reformatting quotes in this way means that the user who posts a message will
   no longer have final say over the final content of the message since it will
   be changed after he or she sends it to JamNNTPd. Since this in principle is
   a bad thing even if quoted text will look a lot better after reformatting,
   this option is turned off by default.

 -noencode

   JamNNTPd by default MIME-encodes headers with non-ascii characters. If you
   use this option, JamNNTPd will instead send the headers as plain 8-bit text.

 -keepsoftcr

   JamNNTPd by default removes the "Soft CR" (0x8d) character from message
   bodies. Use this option to disable that behaviour.

 -notzutc

   JamNNTPd normally writes the timezone into a TZUTC kludge when a message
   is posted. You can use this line if you don't want to create TZUTC kludges.

 -p <port>

   Set the port where JamNNTPd listens for connections. The default is 5000.

 -m <maxconn>

   The maximum allowed number of connections at one time. The default is 5.

 -def_flowed on/off
 -def_showto on/off

   Sets the default of the flowed and showto settings (if no default is
   specified on the commandline, both will be on by default)

    flowed: If flowed is on, JamNNTPd will use format=flowed (section 6.5),
            otherwise it will wrap long lines to a fixed width.

    showto: Since there is no receiver for news messages, JamNNTPd can display
            the receiver name as a part of the sender name. With this option,
            this behaviour can be turned on or off.

   These can be modified by the user by loggin in with parameters (section 4.4)

 -origin <origin>

   Normally JamNNTPd uses the text found in the Organization header line as the
   Origin line text in posted messages. You can use this switch to override the
   Organization line and set your own origin for all posted messages.

 -g <groupsfile>
 -a <allowfile>
 -u <usersfile>
 -x <xlatfile>

   Use these to override the default locations of the config files.

 -l <logfile>

   Use this to override the default location of the log file.

4.2 Access rights
-----------------
Access rights in JamNNTPd is based on access groups. Every newsgroup in
JamNNTPd belongs to an acess group. Access groups are named using one letter,
typically A to Z (access groups are case-insensitive).

When a user connects to the server, he/she gets access to two set of access
groups. The first set of groups are for read access and the second set of
groups are for post access. Users are only allowed to read newsgroups that
belong to an access group they have read access to and are only allowed to
post to newsgroups belong to an access group that they have post access to.
The default access groups for users are configured in the "allow" file.

A user might get access to additional access groups if he/she logs in to the
server using the AUTHINFO command. The groups associated with a user are
defined in the "users" file.

4.3 Configuration files
-----------------------
JamNNTPd uses four configuration files:

1) In the "groups" file, the JAM areas that JamNNTPd should provide as
   newsgroups are configured.

2) In the "allow" file, the IP numbers that are allowed to use the server
   are listed. Here you also set the default access rights for users before
   they log in.

3) In the "users" file, you can list users that should be given access to
   additional groups if they log in.

4) In the "xlat" file, you can configure translation between the different
   character sets used in your JAM messagebase and your newsreader.

The format of these files can be seen in the example configuration files.
You do not need to restart JamNNTPd if you change them since they are read
every time a new connection to the server is made.

4.4 Logging in with parameters
------------------------------
Since it is likely that all users will not prefer the same settings for the
flowed and showto options (see section 4.1), these can be modified by the
user by logging in with parameters.

If you want to set these options, login using a login name of this format:

 username/option1=on,option2=off

If you want to set options without logging in, just omit the user name and
enter anything as your password.

Examples:

 billing/showto=off
 billing/flowed=on,showto=on
 /flowed=off

5. Compilation
==============
JamNNTPd should compile with most compilers. I use gcc under Linux and
gcc-mingw32 under Windows. To compile JamNNTPd, go to the src directory
and type either "make linux" or "make win32" depending on what platform
you are compiling JamNNTPd on. After a successful compilation, you will
find a file called "jamnntpd" or "jamnntpd.exe" in the src directory.

6. Compatibility
================

6.1 The NNTP protocol
---------------------
JamNNTPd supports most of the basic NNTP protocol as specified in RFC-977.
The commands IHAVE, NEWGROUPS and NEWNEWS are not implemented, but at least
give valid response codes if a newsreader tries to use them. JamNNTPd also
supports the XOVER and AUTHINFO commands as specified in RFC-2980. XOVER
never sends information about the line counts and byte counts of messages.

6.2 Format of news messages
---------------------------
JamNNTPd probably breaks the RFC-1036 specification on some minor points,
but seems to work well enough with most newsreaders.

MIME is supported. Headers with non-ascii characters are encoded using
quoted-printable or base64 unless disabled with -noencode. Message bodies
are always sent as 7bit or 8bit. The charset is always set to "us-ascii"
if a message does not contain non-ascii characters.

Posted messages can either be in plain text (8bit or 7bit) or encoded
with quoted-printable of base64. Posted messages are only accepted if they
are in the format text/plain (i. e. HTML and multipart messages will be
rejected). Crossposted messages will be rejected. Messages longer than
20 000 bytes will also be rejected.

6.3 MSGID / Message-ID
----------------------
JamNNTPd does not present the MSGIDs found in the JAM messagebase as
Message-IDs to the newsreader, but rather uses its own dummy Message-IDs
instead. The references line in followups will be replaced by the proper
REPLY line.

6.4 REPLYADDR
-------------
When a message is posted to a JAM messagebase, JamNNTPd converts the original
from address to a REPLYADDR kludge. According to FSC-0035, REPLYADDR kludges
should be accompanied by a REPLYTO line. JamNNTPd does not create a REPLYTO
line, but this should not be a major problem. If you want to disable the
REPLYADDR kludge altogether, use the -noreplyaddr option.

6.5 format=flowed
-----------------
A recent extension to MIME (RFC-2646) specifies a format for "flowed" text,
i. e. text that only has line breaks between paragraphs and not after every
line. This format is more well-suited for fidonet messages than the
traditional format since fidonet has always used "flowed" text.

JamNNTPd uses format=flowed unless disabled with the -def_flowed switch or
with login parameters, and it is preferable to use a newsreader that also 
supports this format. Unfortunately, only few newsreaders support this format 
today. JamNNTPd will also work with other newsreaders, but messages will look 
slighty worse both on the NNTP and fidonet side.

6.6 Character set translation
-----------------------------
JamNNTPd has good support for character sets. The character set translation
is configured in the "xlat" file and uses CHS files in the GoldED+ format for
the actual translation. Extendended CHS files with 256 character translations
are supported and a character may be translated to up to four characters.

6.7 Tested newsreaders
----------------------
JamNNTPd has been found to work with the following newsreaders:

   Mozilla 1.4.1
   Outlook Express 6.00.2800.1106
   KNode 0.7.2
   Xnews 5.04.25
   40tude Dialog 2.0.7.1
   Forte Free Agent 1.93/32.576

Of these, only Mozilla seems to support format=flowed.

7. Todo
=======
Here are some improvements that maybe ought to be done:

 * A configuration file instead of lots of commandline switches
 * Netmail handling

But it is pointless for me to spend time on these improvements unless someone
out there needs them, because I don't.


