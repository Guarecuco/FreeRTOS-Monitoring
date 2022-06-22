import socket

 

localIP     = "0.0.0.0"

localPort   = 3999

bufferSize  = 1024

 
 # Create a datagram socket
UDPServerSocket = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)

 
# Bind to address and ip
UDPServerSocket.bind((localIP, localPort))

print("UDP server up and listening")

# Listen for incoming datagrams

while(True):

    bytesAddressPair = UDPServerSocket.recvfrom(bufferSize)
    message = bytesAddressPair[0]
    address = bytesAddressPair[1]
    #clientMsg = "Message from Client:{}".format(message)
    clientMsg = str(message, 'utf-8')
    clientIP  = "Client IP Address:{}".format(address)
    
    #print(message)
    print(clientMsg)
    #print(clientIP)