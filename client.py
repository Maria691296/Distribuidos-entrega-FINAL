from enum import Enum
import threading
import argparse
import socket
import time
import sys

SO_MAX_CONN = 20
SOCK_TIMEOUT = 1
MAX_STR_LEN = 256

class client :

    # ******************** TYPES *********************
    # *
    # * @brief Return codes for the protocol methods
    class RC(Enum) :
        OK = 0
        ERROR = 1
        USER_ERROR = 2

    # ****************** ATTRIBUTES ******************
    # Direccion del servidor
    _server = None
    _port = -1

    # Elementos del cliente
    _client_sock = None

    # Elementos del client-side-server
    _hilo = None
    _server_port = None
    _server_sock = None
    _is_server_open = False

    # Usuario que utiliza la interfaz
    _user = None


    # Métodos de comunicación

    # Envía cadenas de 256 bytes acabadas en '\0', trunca cadenas más largas y las corrige
    @staticmethod
    def _sendMessage (socket, cadena):
        message = str(cadena)

        # Le añadimos el delimitador a las cadenas que no lo tengan al final
        if message[-1] != '\0':
            message += '\0'

        # No permitimos mandar nada con más de 255+1 bytes de longitud
        if len(message) >= MAX_STR_LEN:
            message = message[:MAX_STR_LEN-1] + "\0"
        try:
            message = message.encode('utf-8')
            socket.sendall(message)
        except Exception as e:
            # print(f"Error inesperado: {e}")
            raise Exception(f"{e}")

    # Lee hasta encontrar '\0' o el caracter 256, limitando las cadenas a 255 para cualquier caso
    @staticmethod
    def _recieveMessage (socket):
        message_len = 0
        message = ''
        try:
            while True:
                msg = socket.recv(1) # Lee un byte en formato de red (python no sabe lo que es)
                message_len += 1
                if (msg == b'\0' or bytes == MAX_STR_LEN): # El formato b'x' traduce x a binario para poder compararlo
                    break
                message += msg.decode() # Lo convierte a un formato legible para Python
            return message
        except Exception as e:
            # print(f"Error inesperado: {e}")
            raise Exception(f"{e}")
    
    # Lee exactamente 1 byte del socket
    @staticmethod
    def _recieveCode (socket):
        try:
            return socket.recv(1)[0] # Hacer [0] sobre un binario te devuelve el valor del binario
        except Exception as e:
            # print(f"Error inesperado: {e}")
            raise Exception(f"{e}")


    # Métodos del cliente

    @staticmethod
    def _client_open ():
        try:
            client._client_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            server_address = (client._server, client._port)
            return client._client_sock.connect(server_address)
        except Exception as e:
            # print(f"Error inesperado: {e}")
            raise Exception(f"{e}")


    @staticmethod
    def _client_close ():
        try:
            client._client_sock.close()
        except Exception as e:
            # print(f"Error inesperado: {e}")
            raise Exception(f"{e}")
        finally:
            client._client_sock = None


    # Métodos del client-side-server

    @staticmethod
    def _server_open ():
        try:
            if not client._is_server_open:
                # Crear el socket del client-side-server
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

                # Le asignamos un puerto disponible aleatorio y obtenemos su puerto
                server_address = ('0.0.0.0', 0)
                sock.bind(server_address)
                sock.listen(SO_MAX_CONN)
                host, client._server_port = sock.getsockname()

                # Establecemos un timeout para hacer un join limpio del hilo
                sock.settimeout(SOCK_TIMEOUT)

                # Asignamos valores a las variables de la clase
                client._is_server_open = True
                client._server_sock = sock

                # Creamos y arrancamos el hilo
                client._hilo = threading.Thread(target = client._server_listen)
                client._hilo.start()
                
            return client._server_port

        except Exception as e:
            print(f"Fallo al abrir el client-side-server: {e}")
            return -1


    @staticmethod
    def _server_listen ():

        while client._is_server_open:

            # Aceptar peticiones, revisando si hay que hacer join
            try:
                connection, client_address = client._server_sock.accept()
            except socket.timeout:
                continue

            # Una vez aceptada la peticion, como procedemos a responder
            try:
                operation = client._recieveMessage(connection)

                # Selector de operación en client-side-server
                if operation == "SEND_MESSAGE":
                    transmitter = client._recieveMessage(connection)
                    recv_mssg_id = client._recieveMessage(connection)
                    message = client._recieveMessage(connection)
                    print(f"c> MESSAGE {recv_mssg_id} FROM {transmitter}")
                    print(f"c> {message}")
                    print(f"c> END")
                

                if operation == "SEND_MESS_ACK":
                    ack_mssg_id = client._recieveMessage(connection)
                    print(f"c> SEND MESSAGE {ack_mssg_id} OK")

            finally:
                connection.close()
        

    @staticmethod
    def _server_close ():
        try:
            if client._is_server_open:
                client._is_server_open = False
                client._hilo.join()
                client._server_sock.close()
        except Exception as e:
            print(f"Hubo un error cerrando el client-side-server:\n{e}")


    # ******************** METHODS *******************
    # *
    # * @param user - User name to register in the system
    # * 
    # * @return OK if successful
    # * @return USER_ERROR if the user is already registered
    # * @return ERROR if another error occurred
    @staticmethod
    def  register(user) :
        try:
            client._client_open()
            client._sendMessage(client._client_sock, "REGISTER")
            client._sendMessage(client._client_sock, user)
            code = client._recieveCode(client._client_sock)
            client._client_close()
        except Exception:
            code = 2

        if code == 0:
            print("c> REGISTER OK")
            return client.RC.OK
        
        elif code == 1:
            print("c> USERNAME IN USE")
            return client.RC.ERROR

        elif code == 2:
            print("c> REGISTER FAIL")
            return client.RC.USER_ERROR
        
        return client.RC.ERROR

    # *
    # 	 * @param user - User name to unregister from the system
    # 	 * 
    # 	 * @return OK if successful
    # 	 * @return USER_ERROR if the user does not exist
    # 	 * @return ERROR if another error occurred
    @staticmethod
    def  unregister(user) :
        try:
            client._client_open()
            client._sendMessage(client._client_sock, "UNREGISTER")
            client._sendMessage(client._client_sock, user)
            code = client._recieveCode(client._client_sock)
            client._client_close()
        except Exception as e:
            code = 2
        
        if code == 0:
            client._user = None
            print("c> UNREGISTER OK")
            return client.RC.OK
        elif code == 1:
            print("c> USER DOES NOT EXIST")
            return client.RC.USER_ERROR
        elif code == 2:
            print("c> UNREGISTER FAIL")
            return client.RC.ERROR
        return client.RC.ERROR
        


    # *
    # * @param user - User name to connect to the system
    # * 
    # * @return OK if successful
    # * @return USER_ERROR if the user does not exist or if it is already connected
    # * @return ERROR if another error occurred
    @staticmethod
    def  connect(user) :
        
        port = client._server_open()

        try:
            client._client_open()
            client._sendMessage(client._client_sock, "CONNECT")
            client._sendMessage(client._client_sock, user)
            client._sendMessage(client._client_sock, port)
            code = client._recieveCode(client._client_sock)
            client._client_close()
        except Exception as e:
            code = 3

        # En cualquier caso de fallo, cerramos la conexion de inmediato
        if code == 0:
            client._user = user
            print("c> CONNECT OK")
            return client.RC.OK
        elif code == 1:
            client._server_close()
            print("c> CONNECT FAIL, USER DOES NOT EXIST")
            return client.RC.USER_ERROR
        elif code == 2:
            client._server_close()
            print("c> USER ALREADY CONNECTED")
            return client.RC.USER_ERROR
        elif code == 3:
            client._server_close()
            print("c> CONNECT FAIL")
            return client.RC.ERROR
        return client.RC.ERROR

    # *
    # * 
    # * @return OK if successful
    # * @return USER_ERROR if the user does not exist or if it is already connected
    # * @return ERROR if another error occurred
    @staticmethod
    def  users() :
        try:
            client._client_open()
            client._sendMessage(client._client_sock, "USERS")
            client._sendMessage(client._client_sock, client._user)
            code = client._recieveCode(client._client_sock)
        except Exception as e:
            code = 2

        if code == 0:
            number_users = int(client._recieveMessage(client._client_sock))
            print(f"c> CONNECTED USERS ({number_users} users connected) OK")
            for i in range(number_users):
                print(client._recieveMessage(client._client_sock))
            client._client_close()
            return client.RC.OK
        
        elif code == 1:
            print("c> CONNECTED USERS FAIL, USER IS NOT CONNECTED")
            client._client_close()
            return client.RC.USER_ERROR
        
        elif code == 2:
            print("c> CONNECTED USERS FAIL")
            client._client_close()
            return client.RC.ERROR
        
        return client.RC.ERROR



    # *
    # * @param user - User name to disconnect from the system
    # * 
    # * @return OK if successful
    # * @return USER_ERROR if the user does not exist
    # * @return ERROR if another error occurred
    @staticmethod
    def  disconnect(user) :
        try:
            client._client_open()
            client._sendMessage(client._client_sock, "DISCONNECT")
            client._sendMessage(client._client_sock, user)
            code = client._recieveCode(client._client_sock)
            client._client_close()
        except Exception as e:
            code = 3


        # Si se realizó con éxito cerramos el hilo
        if code == 0:
            client._server_close()
            print("c> DISCONNECT OK")
            return client.RC.OK
            
        if code == 1:
            print("c> DISCONNECT FAIL, USER DOES NOT EXIST")
            return client.RC.USER_ERROR

        if code == 2:
            print("c> DISCONNECT FAIL, USER NOT CONNECTED")
            return client.RC.USER_ERROR

        if code == 3:
            print("c> DISCONNECT FAIL")
            return client.RC.ERROR

        return client.RC.ERROR

    # *
    # * @param user    - Receiver user name
    # * @param message - Message to be sent
    # * 
    # * @return OK if the server had successfully delivered the message
    # * @return USER_ERROR if the user is not connected (the message is queued for delivery)
    # * @return ERROR the user does not exist or another error occurred
    # TODO PREGUNTAR SOBRE LOS CODIGOS DE RETORNO (CASOS) Y EL ORDEN DE OPERACIONES DEL SEND EN SERVER.C
    # TODO PREGUNTAR SOBRE LOS CODIGOS DE RETORNO (CASOS) Y EL ORDEN DE OPERACIONES DEL SEND EN SERVER.C
    # TODO PREGUNTAR SOBRE LOS CODIGOS DE RETORNO (CASOS) Y EL ORDEN DE OPERACIONES DEL SEND EN SERVER.C
    @staticmethod
    def  send(user,  message) :
        try:
            client._client_open()
            client._sendMessage(client._client_sock, "SEND")
            client._sendMessage(client._client_sock, client._user)
            client._sendMessage(client._client_sock, user)
            client._sendMessage(client._client_sock, message)
            code = client._recieveCode(client._client_sock)
            print(code)
        except Exception:
            code = 2
        

        if code == 0:
            message_id = int(client._recieveMessage(client._client_sock))
            print(f"c> SEND OK - MESSAGE {message_id}")
        if code == 1:
            print("c> SEND FAIL, USER DOES NOT EXIST")
        if code == 2:
            print("c> SEND FAIL")
        
        client._client_close()
        return client.RC.ERROR

    # *
    # * @param user    - Receiver user name
    # * @param file    - file  to be sent
    # * @param message - Message to be sent
    # * 
    # * @return OK if the server had successfully delivered the message
    # * @return USER_ERROR if the user is not connected (the message is queued for delivery)
    # * @return ERROR the user does not exist or another error occurred
    @staticmethod
    def  sendAttach(user,  file,  message) :
        #  Write your code here
        return client.RC.ERROR

    # *
    # **
    # * @brief Command interpreter for the client. It calls the protocol functions.
    @staticmethod
    def shell():

        while (True) :
            try :
                command = input("c> ")
                line = command.split(" ")
                if (len(line) > 0):

                    line[0] = line[0].upper()

                    if (line[0]=="REGISTER") :
                        if (len(line) == 2) :
                            client.register(line[1])
                        else :
                            print("Syntax error. Usage: REGISTER <userName>")

                    elif(line[0]=="UNREGISTER") :
                        if (len(line) == 2) :
                            client.unregister(line[1])
                        else :
                            print("Syntax error. Usage: UNREGISTER <userName>")

                    elif(line[0]=="CONNECT") :
                        if (len(line) == 2) :
                            client.connect(line[1])
                        else :
                            print("Syntax error. Usage: CONNECT <userName>")

                    elif(line[0]=="DISCONNECT") :
                        if (len(line) == 2) :
                            client.disconnect(line[1])
                        else :
                            print("Syntax error. Usage: DISCONNECT <userName>")

                    elif(line[0]=="USERS") :
                        if (len(line) == 1) :
                            client.users()
                        else :
                            print("Syntax error. Usage: CONNECTED_USERS <userName>")

                    elif(line[0]=="SEND") :
                        if (len(line) >= 3) :
                            #  Remove first two words
                            message = ' '.join(line[2:])
                            client.send(line[1], message)
                        else :
                            print("Syntax error. Usage: SEND <userName> <message>")

                    elif(line[0]=="SENDATTACH") :
                        if (len(line) >= 4) :
                            #  Remove first two words
                            message = ' '.join(line[3:])
                            client.sendAttach(line[1], line[2], message)
                        else :
                            print("Syntax error. Usage: SENDATTACH <userName> <filename> <message>")

                    elif(line[0]=="QUIT") :
                        if (len(line) == 1) :
                            client._server_close() # CIERRA EL HILO POR SEGURIDAD
                            break
                        else :
                            print("Syntax error. Use: QUIT")
                    else :
                        print("Error: command " + line[0] + " not valid.")
            except Exception as e:
                print("Exception: " + str(e))

    # *
    # * @brief Prints program usage
    @staticmethod
    def usage() :
        print("Usage: python3 client.py -s <server> -p <port>")


    # *
    # * @brief Parses program execution arguments
    @staticmethod
    def  parseArguments(argv) :
        parser = argparse.ArgumentParser()
        parser.add_argument('-s', type=str, required=True, help='Server IP')
        parser.add_argument('-p', type=int, required=True, help='Server Port')
        args = parser.parse_args()

        if (args.s is None):
            parser.error("Usage: python3 client.py -s <server> -p <port>")
            return False

        if ((args.p < 1024) or (args.p > 65535)):
            parser.error("Error: Port must be in the range 1024 <= port <= 65535");
            return False
        
        client._server = args.s
        client._port = args.p

        return True


    # ******************** MAIN *********************
    @staticmethod
    def main(argv) :
        if (not client.parseArguments(argv)) :
            client.usage()
            return

        #  Write code here
        client.shell()
        print("+++ FINISHED +++")
    
    @staticmethod
    def test():
        client.parseArguments([])
        client._client_open()
        client._sendMessage(client._client_sock, "CONNECT")
        client._sendMessage(client._client_sock, "USUARIO")
        client._sendMessage(client._client_sock, str(123))
        client._client_close()
        return 0

    

if __name__=="__main__":
    client.main([])
