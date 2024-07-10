# shell

### Búsqueda en $PATH

>**¿cuáles son las diferencias entre la syscall execve(2) y la familia de wrappers proporcionados por la librería estándar de C (libc) exec(3)?**

La principal diferencia es la cantidad, la forma y la estructura que reciben los parámetros cada función. Por ejemplo *execve(2)* proporciona mayor control y flexibilidad de los argumentos y variables de entornos pero a cambio esto es un proceso más manual que con por ejemplo *execl(3)* la cual es un wrapper y recibe una lista de argumentos variables pero no permite cambiar variables de entornos de forma directa

>**¿Puede la llamada a exec(3) fallar? ¿Cómo se comporta la implementación de la shell en ese caso?**

Sí, exec(3) puede fallar y si lo hace, establece la variable errno en consecuencia. Nuestra implementación lo que hace es tambien setear el errno con un mensaje descriptivo y luego salir con _exit(2)

---

### Procesos en segundo plano
>**Detallar cuál es el mecanismo utilizado para implementar procesos en segundo plano.**
1. Se valida que el último proceso ejecutado es de tipo `BACK`.
   - En caso de serlo se guarda un puntero al proceso en una variable global y se retorna de la función (salteando el wait).
    - En caso de no serlo entonces se aplica el wait para un proceso común donde luego se libera la memoria de la estructura.
1. En cada llamada a run_cmd, se verifica que el puntero a `proc_back` guarde un puntero válido.
    - Si es NULL entonces no hay un proceso corriendo en el background.
    - Si es distinto de NULL entonces se hace un `waitpid` usando `WNOHANG` donde se verifica si el proceso corriendo en back terminó. En caso de haber terminado entonces se libera la memoria y se imprime por pantalla la finalización del proceso.

>**Explicar detalladamente el mecanismo completo utilizado.**
* Al iniciar la shell se invoca a `init_signal_handling()` que como dice el nombre se encarga de inicializar el manejo de señales para los procesos que corren en segundo plano. Esta función crea el stack alterno para la función de handling, inicializa el `struct sigaction` pasandole el handler con sus determinados flags `SA_ONSTACK | SA_RESTART`, llenar su mask para evitar que se interrumpa el handling por otra señal y aplicarla al proceso de la shell.
* Cada vez que se cree un proceso hijo se le asigna un group id igual al de su pid para que nuestro handler no lo trate como un proceso en back (quienes no tienen alterado su group id y es igual al de su proceso padre) y entonces no hace nada cuando estos terminan.

>**¿Por qué es necesario el uso de señales?**
* Para la comunicación entre procesos con interrupciones. En este caso queremos que los procesos hijos interrumpan al proceso padre para imprimir por pantalla la finalización del mismo.
---

### Flujo estándar
>Investigar el significado de `2>&1`, explicar cómo funciona su forma general

`2>&1` es una forma de redirigir stderr hacia la salida estandar mediante el
comando `>`, en la cual los carácteres `1` y `2` representan a `stdout` y `stderr`
respectivamente. Anteponer el carácter `2` al comando `>` especifica al shell que 
debe redirigir lo que se encuentre en la salida de errores hacia el descriptor
esperado, que en este caso es `&1`.
A su vez, el carácter `&` se utiliza para indicar que, en este caso, el
file descriptor referido mediante `1` es la salida estándar y no un archivo
llamado '1'; de tal forma, si no se hace explícita la salida estándar mediante
`&1`, el flujo de `stderr` se escribiría en un fichero de nombre '1', que en caso de
no existir sería creado tras la ejecución del comando.

Por ejemplo, si se cuenta con un programa llamado `foo` y se quisiera registrar
su salida de errores en un archivo, llámese `err.txt`, aplicaría la siguiente
versión del comando anterior:
`foo 2>err.xtx`

>Mostrar qué sucede con la salida de cat out.txt en el ejemplo

Si se corre la siguiente secuencia de comandos
`ls -C /home /noexiste >out.txt 2>&1`

el contenido del archivo `out.txt` será:

`ls: cannot access '/noexiste': No such file or directory
/home:
user''`

Esto ocurre porque internamente primero se asigna `stdout` a `out.txt` (es decir, en out.txt se va a escribir lo que se envío a stdout), y luego stderr se asigna a stdout, por lo que `out.txt` termina conteniendo todo lo
escrito en `stdout`, que es lo naturamente escrito en la salida estándar por el
comando `ls -C /home` y lo escrito en stderr por la parte de `/noexiste`.

>Luego, repetirlo invirtiendo el orden de las redirecciones (es decir, 2>&1 >out.txt).
¿Cambió algo? Compararlo con el comportamiento en bash(1)?

El flujo de stderr se imprime por consola, mientras que lo enviado por `stdout`
termina siendo escrito en el fichero out.txt. Dado que al momento de ejecutarse
el primer comando (el situado sintácticamente al principio de la secuencia)
stdout envía su flujo hacia la terminal, stderr es impreso mediante esa vía, y
luego se redirige el flujo de stdout al archo al cual se lo asigna mediante la
instrucción ` >out.txt`

---

### Tuberías múltiples
>Investigar qué ocurre con el exit code reportado por la shell si se ejecuta un pipe

>¿Cambia en algo?

Si ninguno de los comandos falla, tanto en el bash como en nuestra implementación, el exit code es 0.

Bash y nuestra shell:
```bash
$ seq 20 | grep 1 | sort -r | head -n 5
19
18
17
16
15
$ echo $?
0
```

>¿Qué ocurre si, en un pipe, alguno de los comandos falla? Mostrar evidencia (e.g. salidas de terminal) de este comportamiento usando bash. Comparar con su implementación.

Si alguno de los comandos falla, en el bash solo se va a modificar el exit code si el comando fallido es el último del pipe dado que se corren de manera secuencial de izquierda a derecha. En nuestra implementación, también se ejecutan los comandos respetando ese orden pero el exit code nunca es modificado cuando se ejecuta un pipe por lo que cuando se de un error este no va a ser reflejado en el exit code.

Bash:
```bash
$ seq 20 | grep 1 | sooort -r | head -n 5 
sooort: no se encontró la orden
$ echo $?
0
```
``` bash
$ seq 20 | grep 1 | sort -r | heaaaad -n 5 
heaaaad: no se encontró la orden
$ echo $?
127
```

Nuestra shell:
```bash
$ seq 20 | grep 1 | sooort -r | head -n 5 
sooort: no se encontró la orden
$ echo $?
0
```
``` bash
$ seq 20 | grep 1 | sort -r | heaaaad -n 5 
heaaaad: no se encontró la orden
$ echo $?
0
```

---

### Variables de entorno temporarias
> **¿Por qué es necesario hacerlo luego de la llamada a fork(2)?**

Es necesario porque queremos que sea una variable efímera solo conocida en el scope del comando que estamos ejecutando. Si se hiciera antes estas variables dejarían de ser temporales y estarían disponibles en todos los comandos que la instancia de la shell realice.

> **En algunos de los wrappers de la familia de funciones de exec(3) (las que finalizan con la letra e), se les puede pasar un tercer argumento (o una lista de argumentos dependiendo del caso), con nuevas variables de entorno para la ejecución de ese proceso. Supongamos, entonces, que en vez de utilizar setenv(3) por cada una de las variables, se guardan en un arreglo y se lo coloca en el tercer argumento de una de las funciones de exec(3).**

>**¿El comportamiento resultante es el mismo que en el primer caso? Explicar qué sucede y por qué.**

El comportamiento no es el mismo dado que el mecanismo de variables de entorno y la forma en que se heredan en un nuevo proceso son diferentes. En la syscall *setenv(3)* se almacenan en el espacio de memoria del proceso actual y estarán disponibles las que se setearon junto con las variables de entorno globales para cualquier función que se ejecute posterior a esta. Mientras que cuando pasas un arreglo de variables de entorno a *exec(3)*, estas serán las únicas variables de entorno que tenga acceso el proceso que se está creando.

>**Describir brevemente (sin implementar) una posible implementación para que el comportamiento sea el mismo.**

Una posible implementación sería pasarle por argumento a *exec(3)* un arreglo de variables de entorno completo con las variables globales y las temporales que se quieren setear, así podría tener un comportamiento similar a *setenv(3)* pero siendo más ineficiente.

---

### Pseudo-variables
> **Investigar al menos otras tres variables mágicas estándar, y describir su propósito**
> **Incluir un ejemplo de su uso en bash (u otra terminal similar).**
1. **$!** : Contiene el PID del último proceso ejecutado en segundo plano
```bash
$ sleep 10 
$ echo $!
1234
```
2. **$_** : Contiene el último argumento del último comando ejecutado
```bash
$ echo hola mundo
hola mundo
$ echo $_
mundo
```
3. **$-** : Contiene las opciones de la shell
```bash
$ echo $-
569JNRXZghikms
```
4.  **$OLDPWD** : Contiene el directorio anterior
```bash
$ cd /tmp
$ echo $OLDPWD
/home/user
```
---

### Comandos built-in
>**¿Entre cd y pwd, alguno de los dos se podría implementar sin necesidad de ser built-in? ¿Por qué? ¿Si la respuesta es sí, cuál es el motivo, entonces, de hacerlo como built-in? (para esta última pregunta pensar en los built-in como true y false)**

Sí, pwd puede correrse usando exec. $PWD es una variable de ambiente que guarda el directorio actual y pwd es un binario en /bin/ que utiliza esta variable. Se suele implementar como built-in porque:
- Es sencillo de implementar.
- No requiere otro proceso (fork -> exec), por lo que es más eficiente.

---

### Historial

---
