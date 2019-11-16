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
    print('Attempting to connect to server')
def parseCommandLine(argv, connectInfo):
    if not commandLineValidation(argv):
        sys.exit(0)

    connectInfo[0] = argv[1]
    if argv[3] == "-l":
        connectInfo[1] = argv[2]
        connectInfo[2] = argv[4]
        return "listdir/"
    connectInfo[2] = argv[5]
    return argv[4]

def sendCommand(connectionSocket, command):
    connectionSocket.sendall(command.encode())

def receiveMessage(connectionSocket):
    return connectionSocket.recv(4, 0)

def main():
    serverName = ""
    controlPort = -1
    dataPort = -1
    connectInfo = [serverName, controlPort, dataPort]
    command = parseCommandLine(sys.argv, connectInfo)

    command += str(connectInfo[2])
    print(command)
    controlSocket = sock.socket(sock.AF_INET, sock.SOCK_STREAM)
    controlSocket.connect((connectInfo[0], int(connectInfo[1])))

    sendCommand(controlSocket, command)

    dataSocket = sock.socket(sock.AF_INET, sock.SOCK_STREAM)
    dataSocket.bind(('', int(connectInfo[2])))
    connectSocket(dataSocket)

    connectionSocket, addr = dataSocket.accept()
    
    message = receiveMessage(connectionSocket)
    message = int.from_bytes(message, byteorder='little')

    print(message)


if __name__ == "__main__":
    main()