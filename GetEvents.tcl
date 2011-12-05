#---------------------------------------------------------------------------+
#                           SOCKET EVENTS SCRIPT                            |
#                            2011 +-+ mables                                |
#                      https://github.com/mables                            |
#---------------------------------------------------------------------------+
#
# Watch for socket events and write them to channel.
#

# Connection variables
set localip ""
set localport 29522
set channel ""
set eventmessage "New file uploaded"
set eventrurl ""

# Get output from socket
proc read_sock {sock} { 
	set message [gets $sock] 
	if {[string match *Event* $message]} {
		# Remove "Event " from message and write to $channel
		set message [string trimleft $message 6]
   		putserv "PRIVMSG $channel \00300$eventmessage\003: $eventurl$message\003"
	} 
} 

# Set connection
set socketvar [socket $localip $localport] 

# Set the socket to readable, and monitor it
fileevent $socketvar readable [list read_sock $socketvar] 

# Set socket options: Flush output from channel after every \n 
fconfigure $socketvar -buffering line 

