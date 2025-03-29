Medidor de temperatura y humedad

Recoge la hora de http://worldtimeapi.org/api/timezone/Europe/Madrid
Recoge la temperatura y humedad de los sensores
Envía con ello cada medida a una api local, que la almacena en una base de datos
La api está en mi proyecto nodejs_mysql_api

DONE - Estaría bien que guardara los Logs que se envían a Serial en un archivo
DONE - Y que los datos de conexión con la BD estuvieran en otro archivo
DONE - También que se pudiera definir el tiempo de ejecución
DONE - Llevar a github el código
DONE - Lo malo es que cada vez la IP de mi API será diferente. HAY QUE CAMBIARLA cada vez (pongo IP fija del equipo que hace de servidor)
DONE - Hacer que envíe una notificación a Telegram
DONE - Hacer que con /temperatura y /humedad en el bot me lleguen esos datos
DONE - Crear función para escribir información (escribe en el Serial, en el log de la SD y en Telegram)

Activarlo con un flag de activo/inactivo usando los botones
