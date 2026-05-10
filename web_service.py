"""
Servicio Web SOAP para normalización de mensajes.
Elimina espacios en blanco repetidos de los mensajes.

Ejecutar con: python3 web_service.py
Requiere:     pip3 install spyne lxml zeep
"""

import re
from wsgiref.simple_server import make_server
from spyne import Application, rpc, ServiceBase, Unicode
from spyne.protocol.soap import Soap11
from spyne.server.wsgi import WsgiApplication

WS_PORT = 8000


class MessageNormalizerService(ServiceBase):

    @rpc(Unicode, _returns=Unicode)
    def normalize(ctx, message):
        if message is None:
            return ""
        
        # Sustituir cualquier secuencia de espacios/tabs por un solo espacio
        normalized = re.sub(r'\s+', ' ', message).strip()
        return normalized


# Configuración de la aplicación SOAP
application = Application(
    services=[MessageNormalizerService],
    tns='ssdd.pf.normalizer',
    in_protocol=Soap11(validator='lxml'),
    out_protocol=Soap11()
)

wsgi_app = WsgiApplication(application)


if __name__ == '__main__':
    server = make_server('0.0.0.0', WS_PORT, wsgi_app)
    print(f"ws> Servicio web SOAP escuchando en http://0.0.0.0:{WS_PORT}")
    print(f"ws> WSDL disponible en http://localhost:{WS_PORT}/?wsdl")
    server.serve_forever()
