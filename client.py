from zeep import Client as SoapClient
from enum import Enum
import threading
import argparse
import socket
import time
import sys

SO_MAX_CONN = 20
SOCK_TIMEOUT = 1
MAX_STR_LEN = 256
FILE_PACKAGE_SIZE = 1024
WS_WSDL = 'http://localhost:8000/?wsdl'

class client :

    # =================================================================================================
    # Retornos de las funcionalidades
    # =================================================================================================

    class RC(Enum) :
        OK = 0
        ERROR = 1
        USER_ERROR = 2


    # =================================================================================================
    # Retornos de las funcionalidades
    # =================================================================================================

    # Datos de la sesión
    
    # Usuario que utiliza la interfaz
    _user = None

    # Listado de usuarios conectados y sus threads de escucha
    _user_list = {}



    # Datos locales

    # Dirección del client-side-client (nuestra direccion actual)
    _local_client_ip = 0
    _local_client_port = 0

    # Dirección del client-side-server (nuestro thread de escucha)
    _local_server_ip = 0
    _local_server_port = 0
    _local_server_sock = None
    _local_server_open = False
    _local_server_thread = None



    # Datos remotos

    # Dirección del server-side-server (server.c)
    _remote_server_ip = 0
    _remote_server_port = 0

    # Dirección del server-side-client (server.c // client.py)
    _remote_client_ip = 0
    _remote_client_port = 0



    # =================================================================================================
    # Métodos de comunicación por sockets
    # =================================================================================================


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

    # Normaliza un mensaje usando el servicio web SOAP
    @staticmethod
    def _normalize_message(message):
        try:
            soap_client = SoapClient(WS_WSDL)
            return soap_client.service.normalize(message)
        except Exception:
            print("c> AVISO: Servicio de normalización no disponible")
            return message


    # =================================================================================================
    # Métodos del cliente
    # =================================================================================================


    @staticmethod
    def _client_open (ip, port):
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            server_address = (ip, port)
            sock.connect(server_address)
            return sock
        except Exception as e:
            # print(f"Error inesperado: {e}")
            raise Exception(f"{e}")


    @staticmethod
    def _client_close (sock):
        try:
            sock.close()
        except Exception as e:
            # print(f"Error inesperado: {e}")
            raise Exception(f"{e}")



    # =================================================================================================
    # Métodos del client-side-server
    # =================================================================================================


    @staticmethod
    def _server_open ():
        try:
            if not client._local_server_open:
                # Crear el socket del client-side-server
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

                # Le asignamos un puerto disponible aleatorio y obtenemos su puerto
                server_address = ('0.0.0.0', 0)
                sock.bind(server_address)
                sock.listen(SO_MAX_CONN)
                host, client._local_server_port = sock.getsockname()

                # Establecemos un timeout para hacer un join limpio del hilo
                sock.settimeout(SOCK_TIMEOUT)

                # Asignamos valores a las variables de la clase
                client._local_server_open = True
                client._local_server_sock = sock

                # Creamos y arrancamos el hilo
                client._local_server_thread = threading.Thread(target = client._server_listen)
                client._local_server_thread.start()
                
            return client._local_server_port

        except Exception as e:
            print(f"Fallo al abrir el client-side-server: {e}")
            return -1

    @staticmethod
    def _server_listen ():

        while client._local_server_open:

            # Aceptar peticiones, revisando si hay que hacer join
            try:
                connection, client_address = client._local_server_sock.accept()
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

                
                if operation == "SEND_MESSAGE_ATTACH":
                    transmitter = client._recieveMessage(connection)
                    recv_mssg_id = client._recieveMessage(connection)
                    message = client._recieveMessage(connection)
                    filename = client._recieveMessage(connection)
                    print(f"c> MESSAGE {recv_mssg_id} FROM {transmitter}")
                    print(f"c> {message}")
                    print(f"c> END")
                    print(f"c> {filename}")


                if operation == "SEND_MESS_ATTACH_ACK":
                    ack_mssg_id = client._recieveMessage(connection)
                    ack_mssg_filename = client._recieveMessage(connection)
                    print(f"c> SENDATTACH MESSAGE {ack_mssg_id} {ack_mssg_filename} OK")
                

                if operation == "GET_FILE":
                    user = client._recieveMessage(connection)
                    filename = client._recieveMessage(connection)
                    print(f"c> CONFIRMACION DE FICHERO???")
                    # TODO ENVIADO DEL FICHERO
                    with open(filename, "rb") as file:
                        while True:
                            chunk = file.read(FILE_PACKAGE_SIZE)
                            if chunk == b'':
                                break
                            connection.sendall(chunk)

            finally:
                connection.close()
        
    @staticmethod
    def _server_close ():
        try:
            if client._local_server_open:
                client._local_server_open = False
                client._local_server_thread.join()
                client._local_server_sock.close()
        except Exception as e:
            print(f"Hubo un error cerrando el client-side-server:\n{e}")



    # =================================================================================================
    # Funcionalidades del cliente
    # =================================================================================================


    @staticmethod
    def  register(user) :
        try:
            sock = client._client_open(client._remote_server_ip, client._remote_server_port)
            client._sendMessage(sock, "REGISTER")
            client._sendMessage(sock, user)
            code = client._recieveCode(sock)
            client._client_close(sock)
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

    @staticmethod
    def  unregister(user) :
        try:
            sock = client._client_open(client._remote_server_ip, client._remote_server_port)
            client._sendMessage(sock, "UNREGISTER")
            client._sendMessage(sock, user)
            code = client._recieveCode(sock)
            client._client_close(sock)
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

    @staticmethod
    def  connect(user) :
        
        port = client._server_open()

        try:
            sock = client._client_open(client._remote_server_ip, client._remote_server_port)
            client._sendMessage(sock, "CONNECT")
            client._sendMessage(sock, user)
            client._sendMessage(sock, port)
            code = client._recieveCode(sock)
            client._client_close(sock)
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
            client._user = user
            print("c> USER ALREADY CONNECTED")
            return client.RC.USER_ERROR
        elif code == 3:
            client._server_close()
            print("c> CONNECT FAIL")
            return client.RC.ERROR
        return client.RC.ERROR

    @staticmethod
    def  users() :
        try:
            sock = client._client_open(client._remote_server_ip, client._remote_server_port)
            client._sendMessage(sock, "USERS")
            client._sendMessage(sock, client._user)
            code = client._recieveCode(sock)
        except Exception as e:
            code = 2

        if code == 0:
            number_users = int(client._recieveMessage(sock))
            print(f"c> CONNECTED USERS ({number_users} users connected) OK")
            # Reinicio de los valores para recibir todos los usuarios del servidor
            user = ""
            ip = ""
            client._user_list = {}

            # Tokenizamos las cadenas de los usuarios y los almacenamos
            for i in range(number_users):
                # Tokenizamos los valores extraídos del usuario
                datos = client._recieveMessage(sock).split("::")
                maxuserlen = max(len(user), len(datos[0]))
                maxiplen = max(len(ip), len(datos[1]))
                user, ip, port = datos[0], datos[1], datos[2]
                client._user_list[user] = {"ip": ip, "port": port}
                # print(f"   {user}   {ip}   {port}") # 
            
            client._client_close(sock)

            # Esta parte es innecesaria, está por estética
            for user_key in client._user_list.keys():
                ip = client._user_list[user_key]["ip"]
                port = client._user_list[user_key]["port"]
                
                userlen = maxuserlen - len(user_key)
                iplen = maxiplen - len(ip)

                user_padding = user_key + (" " * userlen)
                ip_padding = ip + (" " * iplen)
                print(f"   {user_padding}   {ip_padding}   {port}")
            
            return client.RC.OK
        
        elif code == 1:
            print("c> CONNECTED USERS FAIL, USER IS NOT CONNECTED")
            client._client_close(sock)
            return client.RC.USER_ERROR
        
        elif code == 2:
            print("c> CONNECTED USERS FAIL")
            client._client_close(sock)
            return client.RC.ERROR
        
        return client.RC.ERROR

    @staticmethod
    def  disconnect(user) :
        try:
            sock = client._client_open(client._remote_server_ip, client._remote_server_port)
            client._sendMessage(sock, "DISCONNECT")
            client._sendMessage(sock, user)
            code = client._recieveCode(sock)
            client._client_close(sock)
            client._server_close()
        except Exception as e:
            code = 3

        if code == 0:
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

    @staticmethod
    def  send(user,  message) :
        if client._user == None:
            print(f"c> TERMINAL DOESN'T KNOW WHICH USER IS SENDING THIS")
            return client.RC.USER_ERROR
        
        try:
            sock = client._client_open(client._remote_server_ip, client._remote_server_port)
            client._sendMessage(sock, "SEND")
            client._sendMessage(sock, client._user)
            client._sendMessage(sock, user)
            client._sendMessage(sock, client._normalize_message(message))
            code = client._recieveCode(sock)
        except Exception:
            code = 2
        
        if code == 0:
            message_id = int(client._recieveMessage(sock))
            print(f"c> SEND OK - MESSAGE {message_id}")
        if code == 1:
            print("c> SEND FAIL, USER DOES NOT EXIST")
        if code == 2:
            print("c> SEND FAIL")
        
        client._client_close(sock)
        return client.RC.ERROR

    @staticmethod
    def  sendAttach(user,  file,  message) :
        if client._user == None:
            print(f"c> TERMINAL DOESN'T KNOW WHICH USER IS SENDING THIS")
            return client.RC.USER_ERROR
        
        try:
            sock = client._client_open(client._remote_server_ip, client._remote_server_port)
            client._sendMessage(sock, "SENDATTACH")
            client._sendMessage(sock, client._user)
            client._sendMessage(sock, user)
            client._sendMessage(sock, client._normalize_message(message))
            client._sendMessage(sock, file)
            code = client._recieveCode(sock)
        except Exception:
            code = 2
        
        if code == 0:
            message_id = int(client._recieveMessage(sock))
            print(f"c> SENDATTACH OK - MESSAGE {message_id}")
        if code == 1:
            print("c> SENDATTACH FAIL, USER DOES NOT EXIST")
        if code == 2:
            print("c> SENDATTACH FAIL")
        
        client._client_close(sock)

        return client.RC.ERROR

    @staticmethod
    def  getFile(user,  filename,  filedestiny) :
        # Obtener la ip del usuario
        ip, port = client._user_list[user]["ip"], int(client._user_list[user]["port"])

        # Abrimos la conexion con el otro usuario
        try:
            sock = client._client_open(ip, port)
            client._sendMessage(sock, "GET_FILE")
            client._sendMessage(sock, client._user)
            client._sendMessage(sock, filename)
            # TODO RECIBIR FICHERO
            with open(filedestiny, "wb") as file:
                while True:
                    chunk = sock.recv(FILE_PACKAGE_SIZE)
                    if chunk == b'':
                        break
                    file.write(chunk)

            client._client_close(sock)
        except Exception as e:
            print(f"Error en getFile: {e}")


    

    # =================================================================================================
    # Shell, usage, parseArguments y main
    # =================================================================================================


    @staticmethod
    def shell():

        while (True) :
            time.sleep(1)
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
                    
                    elif(line[0]=="GETFILE") :
                        if (len(line) >= 4) :
                            #  Remove first two words
                            client.getFile(line[1], line[2], line[3])
                        else :
                            print("Syntax error. Usage: GETFILE <userName> <filename> <localfilename>")

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

    @staticmethod
    def usage() :
        print("Usage: python3 client.py -s <server> -p <port>")

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
        
        client._remote_server_ip = args.s
        client._remote_server_port = args.p

        return True

    @staticmethod
    def main(argv) :
        if (not client.parseArguments(argv)) :
            client.usage()
            return

        #  Write code here
        client.shell()
        print("+++ FINISHED +++")

    # Fin de class client


if __name__=="__main__":
    client.main([])