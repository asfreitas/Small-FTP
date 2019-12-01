#
# Name: Andrew Freitas
# OSID: Freitand
# Project 2
# 12/01/2019
#

import sys as sys
import socket as sock

# Checks command line input for proper length only dependent on type
# Returns true or false
def commandLineValidation(argv):
    if argv[3] == "-l" and len(argv) != 5:
        print("Not an appropriate amount of arguments for requesting the directory listing.")
        return False
    elif argv[3] == "-g" and len(argv) != 6:
        print("Not an appropriate amount of arguments for re")
        return False
    return True
# Begins listening to number of sockets input
def connectSocket(socket):
    socket.listen(1)

# Parses some of the command line information into a more readable array
# Returns the proper command parsed a bit for sending
def parseCommandLine(argv, connectInfo):
 #   if not commandLineValidation(argv):
  #      sys.exit(0)

    connectInfo[0] = argv[1]
    if argv[3] == "-l":
        connectInfo[1] = argv[2]
        connectInfo[2] = argv[4]
        return "-l/"
    if argv[3] == "-g":
        connectInfo[1] = argv[2]
        connectInfo[2] = argv[5]
        connectInfo[3] = argv[4]
        return "-g/" + argv[4] + "/"       

def sendCommand(connectionSocket, command):
    connectionSocket.sendall(command.encode())

def receiveMessage(connectionSocket, size):
    return connectionSocket.recv(size)
# Gets the size of file being sent from server
# Returns an integer representing the amount of bytes sent
def getSize(connectionSocket):
    size = receiveMessage(connectionSocket, 4)
    size = int.from_bytes(size, byteorder='little')
    return size
# Receives the list of files from a connection
# Returns the message unparsed
def receiveFileList(connectionSocket):
                
    size = getSize(connectionSocket)
    message = receiveMessage(connectionSocket, size)
    message = message.decode()
    return message
# Parses the message received from the server and prints
# to the console window
def printDirectoryStructure(directories, connectInfo):
    listing = directories.split('/')
    for file in listing:
        print(file)
# Receives a file and writes to a new file
def receiveFile(connectionSocket, connectInfo):
    newfile = open(connectInfo[3], "w")
    size = getSize(connectionSocket)
    bytesreceived = 0
    while(bytesreceived < size):
        buffer = receiveMessage(connectionSocket, 512)
        print(buffer)
        newfile.write(buffer.decode())
        bytesreceived += len(buffer)


def main():
    serverName = ""
    controlPort = -1
    dataPort = -1
    file = ""
    connectInfo = [serverName, controlPort, dataPort, file]
    command = parseCommandLine(sys.argv, connectInfo)


    dataSocket = sock.socket(sock.AF_INET, sock.SOCK_STREAM)
    dataSocket.bind(('', int(connectInfo[2])))
    connectSocket(dataSocket)

    commandPlusPort = command + str(connectInfo[2])
    controlSocket = sock.socket(sock.AF_INET, sock.SOCK_STREAM)
    controlSocket.connect((connectInfo[0], int(connectInfo[1])))

    sendCommand(controlSocket, commandPlusPort)
    checkCommand = controlSocket.recv(2).decode()
    controlSocket.close()

    if checkCommand == "ok":

        connectionSocket, addr = dataSocket.accept()
        
        if command == "-l/":
            print("Receiving directory structure from " + connectInfo[0] + ":" + connectInfo[2])
            message = receiveFileList(connectionSocket)
            printDirectoryStructure(message, connectInfo)
        else:
            receiveFile(connectionSocket, connectInfo)


if __name__ == "__main__":
    main()