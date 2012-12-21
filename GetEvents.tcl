#---------------------------------------------------------------------------+
#                           SOCKET EVENTS SCRIPT                            |
#                            2011 +-+ mables                                |
#                      https://github.com/mables                            |
#---------------------------------------------------------------------------+
#
# Watch for socket events and write them to channel.
#

# Freebsd pkg: /usr/ports/net/tcludp
package require udp

# Connection variables
set localport 20000
set channel "#channel"
set eventmessage "New file uploaded"
set eventurl "http://example.com/"



# Get output from socket
proc readSocket {sock} {
	global channel
	global eventmessage
	global eventurl
	set message [read $sock]
	set peer [fconfigure $sock -peer]
	#putlog "UDP message from $peer: $message (Length: [string length $message])"

	if {[regexp Event $message]} {
		set trimmessage [string range $message 6 31]
		set msgString "\00300$eventmessage\003: $eventurl$trimmessage\003"
		putserv "PRIVMSG $channel $msgString"
	}
	if {[regexp IFORUM $message]} {
		set trimmessage [string range $message 7 250]
		set msgString "\00300$trimmessage\003"
		putserv "PRIVMSG $channel $msgString"
	}
	flush $sock
	return 0
}

set socketvar [udp_open $localport]
fconfigure $socketvar -buffering line
fileevent $socketvar readable [list readSocket $socketvar]
putlog "Listening on UDP port [fconfigure $socketvar -myport]"

putlog "GetEvents addon by mables"
