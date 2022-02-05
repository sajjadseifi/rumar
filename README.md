# Rumar

#
Peer to Peer server client application
#
Each peer must send a request to another peer to be added to the network
#
By order peer ip:port
#
Will be sent to the desired peer. The recipient will be like the sender of the server and the sender will be the client
#
If the server is not available, it will add it to its list of connections and send all its messages to it.
#
By sending a message by command
rumar message
The message will go to all peers within peer connections (peer-to-peer clients). And if the message is available, the message will not be added to the list.
#
If a message is received, each peer will send it to all its clients, and in this way, rumors will be spread.

# Rules
<ul>
    <li>peer {ip:port}</li>
    <li>ruamr {text-message}</li>
</ul>
