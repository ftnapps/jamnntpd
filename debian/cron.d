#
# Regular cron jobs for the jamnntpd package
#
0 4	* * *	root	[ -x /usr/bin/jamnntpd_maintenance ] && /usr/bin/jamnntpd_maintenance
