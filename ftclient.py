import sys as sys
import socket as sock

def commandLineValidation(argv):
    if argv[3] == "-l" and len(argv) != 5:
        print("Not an appropriate amount of arguments for requesting the directory listing.")
        return False
    elif argv[3] == "-g" and len(argv) != 6:
        print("Not an appropriate amount of arguments for re")
        return False
    return True

def connectSocket(socket):
    socket.listen(1)
def parseCommandLine(argv, connectInfo):
    if not commandLineValidation(argv):
        sys.exit(0)

    connectInfo[0] = argv[1]
    if argv[3] == "-l":
        connectInfo[1] = argv[2]
        connectInfo[2] = argv[4]
        return "-l/"
    connectInfo[2] = argv[5]
    return argv[4]

def sendCommand(connectionSocket, command):
    connectionSocket.sendall(command.encode())

def receiveMessage(connectionSocket, size):
    return connectionSocket.recv(size)

def receiveFileList(connectionSocket):
                
    size = receiveMessage(connectionSocket, 4)
    size = int.from_bytes(size, byteorder='little')
    message = receiveMessage(connectionSocket, size)
    message = message.decode()
    return message

def printDirectoryStructure(directories, connectInfo):
    print("Receiving directory structure from " + connectInfo[0] + ":" + connectInfo[2])
    listing = directories.split('/')
    for file in listing:
        print(file)
    
def main():
    serverName = ""
    controlPort = -1
    dataPort = -1
    connectInfo = [serverName, controlPort, dataPort]
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
            message = receiveFileList(connectionSocket)
            printDirectoryStructure(message, connectInfo)


if __name__ == "__main__":
    main()