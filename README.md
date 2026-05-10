Obtención del ejecutable

Para compilarlo, necesitamos las condiciones del entorno docker propuesto para la práctica de AG.
Para cada dispositivo o working directory ejecutamos en la terminal (una vez por sitio donde se necesiten los ejecutables), dentro de cualquiera de los contenedores lo siguiente:
&> make



Forma de ejecutar

Una vez compilado, dependiendo del rol que vaya a tomar el contenedor docker actual, tomamos uno de los 3 caminos para establecer los roles.

1) CLIENTE: Ejecutar el script que instala pip3, las dependencias de spyne, zeep y lxml, inicia el servidor web service de SOAP11 y finalmente ejecuta el cliente que apunta al servidor cuya dirección está especificada como parámetros.
      &> ./prep_client [DIRECCIÓN DEL SERVIDOR] [PUERTO DEL SERVIDOR]

2) SERVIDOR: Establece la variable de entorno y ejecuta directamente el servidor
      &> ./prep_servidor [DIRECCIÓN DEL SERVIDOR RPC] [PUERTO DE ALOJAMIENTO DEL SERVIDOR]
Una vez compilado, dependiendo del rol que vaya a tomar el contenedor docker actual, tomamos uno de los 3 caminos para establecer los roles.

3) SERVIDOR DE LOGS RPC: No necesita más dependencias de las que hay instaladas ya en el docker, ejecutamos directamente
      &> ./log_client
