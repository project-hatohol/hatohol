#
# Regular cron jobs for the hatohol package
#
0 4	* * *	root	[ -x /usr/bin/hatohol_maintenance ] && /usr/bin/hatohol_maintenance
