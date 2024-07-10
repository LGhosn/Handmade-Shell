---
copyright: Patricio Iribarne Catella
---

# TP1: Intérprete de comandos - _shell_ - Sisop - FIUBA


## Introducción

**AVISO**: antes de comenzar, verificar que se tiene instalado [el software necesario](https://fisop.github.io/website/kit/).

En este trabajo se va a desarrollar la funcionalidad mínima que caracteriza a un intérprete de comandos *shell* similar a lo que realizan `bash`, `zsh`, `fish`.

La implementación debe realizarse en C11 y POSIX.1-2008. *(Estas siglas hacen referencia a la versión del lenguaje C utilizada y del estándar de syscalls Unix empleado. Las versiones modernas de GCC y Linux cumplen con ambos requerimientos.)*

## Implementación

### Parte 1: Invocación de comandos

#### Búsqueda en *$PATH*

Los comandos que más se utilizan, como `ls`, `echo`, etc., están guardados (sus binarios), en el directorio `/bin`. Por este motivo existe una variable de entorno llamada `$PATH`, en la cual se declaran las rutas más usualmente accedidas por el sistema operativo (ejecutar: `echo $PATH` para ver la totalidad de las rutas almacenadas). Por ejemplo:

```bash
$ uptime
 05:45:25 up 5 days, 12:02,  5 users,  load average: ...
```

Asimismo, es deseable, la funcionalidad de poder pasarle _argumentos_ al momento de querer ejecutar dichos comandos. Los argumentos pasados al programa de esta forma, se guardan en la famosa variable **char* argv[]**, junto con cuántos fueron en **int argc**, declaradas en la función `main` de cualquier programa en *C*.

```bash
$ df -H /tmp
Filesystem      Size  Used Avail Use% Mounted on
tmpfs           8.3G  2.0M  8.3G   1% /tmp
```

**Tareas**
  - Soportar la ejecución de binarios
    - Con búsqueda en `$PATH`
    - Con y sin argumentos
  - **Responder**: ¿cuáles son las diferencias entre la _syscall_ `execve(2)` y la familia de _wrappers_ proporcionados por la librería estándar de *C* (_libc_) `exec(3)`?
  - **Responder**: ¿Puede la llamada a `exec(3)` fallar? ¿Cómo se comporta la implementación de la _shell_ en ese caso?

**Función sugerida:**  `execvp(3)`

**Archivo:** `exec_cmd()` en _exec.c_

#### Procesos en segundo plano

Los procesos en segundo plano o procesos en el "fondo", o _background_, son muy útiles a la hora de ejecutar comandos que no queremos esperar a que terminen para que la *shell* nos devuelva el *prompt* nuevamente. Por ejemplo, si queremos ver algún documento *.pdf* o una imagen y queremos seguir trabajando en la terminal sin tener que abrir una nueva.

```bash
$ evince file.pdf &
 [PID=2489]

$ ls /home
patricio
```

Sólo se pide la implementación de un proceso en segundo plano. No es necesario que se notifique de la terminación del mismo por medio de mensajes en la _shell_.

Sin embargo, la _shell_ deberá **esperar oportunamente** a los procesos en segundo plano. Esto puede hacerse sincrónicamente antes de mostrar cada _prompt_, con el objetivo de que en una ejecución normal _no se dejen procesos huérfanos_.

**Tareas**
  - Soportar procesos en segundo plano
    - Sin notificación de procesos terminados
    - Esperando _oportunísticamente_ con cada _prompt_ a cada proceso
  - **Responder**: Detallar cuál es el mecanismo utilizado para implementar procesos en segundo plano.

**Ayuda:** Leer el funcionamiento del flag `WNOHANG` de la syscall `wait(2)`

#### Resumen

Al finalizar la parte 1 la _shell_ debe poder:
- Invocar programas y permitir pasarles argumentos.
- Esperar correctamente a la ejecución de los programas.
- Ejecutar procesos en segundo plano.
- Esperar _oportunísticamente_ a los procesos en segundo plano antes de cada prompt.


### Parte 2: Redirecciones

**AVISO**: El esqueleto no soporta espacios entre los caracteres de _redirección_ y
el nombre del archivo proporcionado.

Es decir, se debe ejecutar **siempre**: `<in.txt` y `>out.txt`.

#### Flujo estándar

La redirección del flujo estándar es una de las cualidades más interesantes y valiosas de una *shell* moderna. Permite, entre otras cosas, almacenar la salida de un programa en un archivo de texto para luego poder analizarla, como así también ejecutar un programa enviándole un archivo como entrada estándar. Existen, básicamente, tres formas de redirección del flujo estándar:

- **<u>Entrada y Salida estándares a archivos</u>** *(`<in.txt` `>out.txt`)*

  Son los operadores clásicos del manejo de la redirección del *stdin* y el *stdout* en archivos de entrada y salida respectivamente. Por ejemplo:

  ```bash
  $ ls /usr
  bin etc games include lib local sbin share src

  $ ls /usr >out1.txt
  $ cat out1.txt
  bin etc games include lib local sbin share src

  $ wc -w <out1.txt
  10

  $ ls -C /sys /noexiste >out2.txt
  ls: cannot access '/noexiste': No such file or directory

  $ cat out2.txt
  /sys:
  block  class  devices	 fs  kernel  module  power

  $ wc -w <out2.txt
  8
  ```

  Se puede ver cómo queda implícito que cuando se utiliza el operador **>** se refiere al *stdout* y cuando se utiliza el **<** se refiere al *stdin*.

- **<u>Error estándar a archivo</u>** *(`2>err.txt`)*

  Es una de las dos formas de redireccionar el _flujo estándar de error_ análogo al caso anterior del flujo de salida estándar en un archivo de texto. Por ejemplo:

  ```bash
  $ ls -C /home /noexiste >out.txt 2>err.txt

  $ cat out.txt
  /home:
  patricio

  $ cat err.txt
  ls: cannot access '/noexiste': No such file or directory
  ```

  Como se puede observar, `ls` no informa ningún error al finalizar, como sí lo hacía en el ejemplo anterior. Su salida estándar de error ha sido redireccionada al archivo *err.txt*

- **<u>Combinar salida y errores</u>** *(`2>&1`)*

  Es la segunda forma de redireccionar el flujo estándar producido por errores en la ejecución de un programa. Su funcionamiento se puede observar a través del siguiente ejemplo:

  ```bash
  $ ls -C /home /noexiste >out.txt 2>&1

  $ cat out.txt
  ---????---
  ```

Existen más tipos de redirecciones que nuestra _shell_ no soportará (e.g. `>>` o `&>`)

**Tareas**
  - Soportar cada una de las **tres formas de redirección** descritas arriba: `>`, `<`, `2>` y `2>&1`.
  - **Responder**: Investigar el significado de `2>&1`, explicar cómo funciona su _forma general_
    - Mostrar qué sucede con la salida de `cat out.txt` en el ejemplo.
    - Luego repetirlo, invirtiendo el orden de las redirecciones (es decir, `2>&1 >out.txt`). ¿Cambió algo? Compararlo con el comportamiento en `bash(1)`.

**Ayuda:** Pueden valerse de las páginas del manual de bash: `man bash`.

**Syscalls sugeridas:** `dup2(2)`, `open(2)`

**Archivo:** `open_redir_fd()` en _exec.c_ y usarla en `exec_cmd()`

#### Tuberías simples *(pipes)*

Al igual que la redirección del flujo estándar hacia archivos, es igual o más importante, la redirección hacia otros programas. La forma de hacer esto en una *shell* es mediante el operador `|` (_pipe_ o _tubería_). De esta forma se pueden concatenar dos o más programas para que la salida estándar de uno se redirija a la entrada estándar del siguiente. Por ejemplo:

```bash
$ ls -l | grep Doc
drwxr-xr-x  7 patricio patricio  4096 mar 26 01:20 Documentos
```

**Tareas**
  - Soportar **pipes** entre dos comandos.
  - La shell debe esperar a que **ambos** procesos terminen antes de devolver el prompt:
	`echo hi | sleep 5` y `sleep 5 | echo hi` _ambos_ deben esperar 5 segundos.
  - Los procesos de cada lado del pipe no deben quedar con _fds_ de más.
  - Los procesos deben ser lanzados _en simultáneo_.

**Syscalls sugeridas:** `pipe(2)`, `dup2(2)`

**Archivo:** `exec_cmd()` en _exec.c_

#### Tuberías múltiples

Extender el funcionamiento de la *shell* para que se puedan ejecutar **n** comandos concatenados.

```bash
$ ls -l | grep Doc | wc
     1       9      64
```

**Tareas**
  - Soportar **múltiples pipes** anidados.
  - **Responder**: Investigar qué ocurre con el _exit code_ reportado por la _shell_ si se ejecuta un pipe
    - ¿Cambia en algo?
    - ¿Qué ocurre si, en un pipe, alguno de los comandos falla? Mostrar evidencia (e.g. salidas de terminal) de este comportamiento usando `bash`. Comparar con su implementación.

**Hint:** Las modificaciones necesarias sólo atañen a la función `parse_line()` en _parsing.c_

#### Resumen

Al finalizar la parte 2 la _shell_ debe poder:
- Redireccionar la entrada y salida estándar de los programas vía `<`, `>` y `2>`.
  - Además se soporta específicamente la redirección de tipo `2>&1`
- Concatenar la ejecución de dos o más programas mediante _pipes_


### Parte 3: Variables de entorno

#### Expansión de variables

Una característica de cualquier intérprete de comandos *shell* es la de expandir variables de entorno (ejecutar: `env` para ver una lista completa de las variables de entorno definidas), como  **PATH**, o **HOME**.

```bash
$ echo $TERM
xterm-16color
```

Las variables de entorno se indican con el caracter `$` antes del nombre, y la _shell_ se encarga de _reemplazar_ en la línea leída todos los tokens que comiencen por `$` por los valores correspondientes del entorno. Esto ocurre _antes_ de que el proceso sea ejecutado.

Las variables vacías y las variables no setteadas *no deben* traducirse a
argumentos en la etapa `exec`. Por ejemplo `echo hola $VARIABLE_VACIA mundo` es
equivalente a `echo "hola" "mundo"`, **dos** argumentos solamente.

**Tareas**
  - Soportar la expansión de variables al ejecutar un comando.
  - Se debe reemplazar las variables que no existan con una cadena vacía (`""`).

**Función sugerida:** `getenv(3)`

**Archivos:** `expand_environ_var()` y `parse_exec()` en _parsing.c_,

#### Variables de entorno temporarias

En esta parte se va a extender la funcionalidad de la _shell_ para que soporte el poder incorporar nuevas variables de entorno a la ejecución de un programa. Cualquier programa que hagamos en C, tiene acceso a todas las variables de entorno definidas mediante la variable externa *environ* (`extern char** environ`).[^noinc]

[^noinc]: No es necesario realizar el _include_ de ningún header para hacer uso de esta variable.

Se pide, entonces, la posibilidad de incorporar de forma dinámica nuevas variables, por ejemplo:

```bash
$ /usr/bin/env
--- todas las variables de entorno definidas hasta el momento ---

$ USER=nadie ENTORNO=nada /usr/bin/env | grep =nad
USER=nadie
ENTORNO=nada
```

**Tareas**
  - Soportar variables de entorno temporales.
  - **Responder**: ¿Por qué es necesario hacerlo luego de la llamada a `fork(2)`?
  - **Responder**: En algunos de los _wrappers_ de la familia de funciones de `exec(3)` (las que finalizan con la letra _e_), se les puede pasar un tercer argumento (o una lista de argumentos dependiendo del caso), con nuevas variables de entorno para la ejecución de ese proceso. Supongamos, entonces, que en vez de utilizar `setenv(3)` por cada una de las variables, se guardan en un arreglo y se lo coloca en el tercer argumento de una de las funciones de `exec(3)`.
    - ¿El comportamiento resultante es el mismo que en el primer caso? Explicar qué sucede y por qué.
    - Describir brevemente (sin implementar) una posible implementación para que el comportamiento sea el mismo.

**Ayuda:** luego de llamar a `fork(2)`, realizar, por cada una de las variables de entorno a agregar, una llamada a `setenv(3)`.

**Función sugerida:** `setenv(3)`

**Archivo:** implementar `set_environ_vars()` en _exec.c_ y usarla en `exec_cmd()`

#### Pseudo-variables

Existen las denominadas variables de entorno *mágicas*, o pseudo-variables. Estas variables son propias de la *shell* (no están formalmente en *environ*) y cambian su valor dinámicamente a lo largo de su ejecución. Implementar **`?`** como única variable *mágica* (describir, también, su propósito).

```bash
$ /bin/true
$ echo $?
0

$ /bin/false
$ echo $?
1
```

**Tareas**
  - Soportar para la  _pseudo-variable_ `$?`.
    - Esto implicará actualizar correctamente la variable _global_ `status` cuando se ejecute un _built-in_ (ya que los mismos no corren en procesos separados).
  - **Responder**: Investigar al menos otras tres variables mágicas estándar, y describir su propósito.
    - Incluir un ejemplo de su uso en `bash` (u otra terminal similar).

**Archivo:** `expand_environ_var()` en _parsing.c_, ver también la variable _global_ `status`.

#### Resumen

Al finalizar la parte 3 la _shell_ debe poder:
- Expandir variables de entorno
 - Incluyendo la pseudo-variable `$?`
- Ejecutar procesos con variables de entorno adicionales


### Parte 4: Comandos _built-in_

Los comandos _built-in_ nos dan la oportunidad de realizar acciones que no siempre podríamos hacer si ejecutáramos ese mismo comando en un proceso separado. Éstos son propios de cada _shell_ aunque existe un estándar generalizado entre los diferentes intérpretes, como por ejemplo `cd` y `exit`.

Es evidente que si `cd` no se realizara en el mismo proceso donde la _shell_ se está ejecutando, no tendría el efecto deseado. Lo mismo se aplica a `exit` y a muchos comandos más ([aquí](https://www.gnu.org/software/bash/manual/bashref.html#Shell-Builtin-Commands) se muestra una lista completa de los comando _built-in_ que soporta _bash_).

**Tareas**
  - Soportar los _built-ins_:
    - `cd` - _change directory_ (cambia el directorio actual)
    - `exit` - exits nicely (termina una terminal de forma _linda_)
    - `pwd` - _print working directory_ (muestra el directorio actual de trabajo)
  - **Responder**: ¿Entre `cd` y `pwd`, alguno de los dos se podría implementar sin necesidad de ser _built-in_? ¿Por qué? ¿Si la respuesta es sí, cuál es el motivo, entonces, de hacerlo como _built-in_? (para esta última pregunta pensar en los _built-in_ como `true` y `false`)

**Funciones sugeridas:** `chdir(3)`, `exit(3)`, `getcwd(3)`

**Archivo:** `cd()`, `exit_shell()` y `pwd()` en _builtin.c_

### Parte 5: Segundo plano avanzado

En la [parte 1](#back) se implementó una solución simple para manejar los procesos en _segundo plano_
preguntando de forma oportunística por la terminación de dichos procesos.

Ahora, queremos mejorar dicha implementación para que el manejo y liberación de _recursos_ del proceso,
ocurra en el _mismo_ momento en que termina.

Para poder realizar ésto, vamos a manejar _señales_ (o `signals`). La señal que nos interesa _atrapar_
es `SIGCHLD`, la cual se genera cada vez que un proceso hijo termina (es decir, llama a `exit(3)`).

El _sistema operativo_ nos da la posibilidad de poder ejecutar lógica _custom_ para cada señal
que se precise manejar de manera particular. Con la _syscall_ `sigaction(2)` vamos a poder configurar
lo que se denomina **handler** (a.k.a una _función_) para dicha señal y liberar los recursos del
proceso en _segundo plano_ que haya finalizado.

Desde el punto de vista del usuario de la _shell_, el comportamiento al final esta tarea,
debería ser el siguiente:

```bash
$ sleep 2 &
PID=2489

$ sleep 5
<pasan dos segundos, y entonces:

==> terminado: PID=2489

:ahora pasan otros tres segundos antes de retornar>
$
```

En otras palabras, se notifica de la terminación en cuanto ocurre, sin esperar al siguiente prompt.

También se observa el comportamiento en la ausencia de un segundo comando en primer plano; simplemente, se escribiría en la línea del _prompt_ actual:

```bash
$ sleep 2 &
PID=2489

$ ==> terminado: PID=2489
 ^
 dos segundos después, se imprime a continuación del prompt
```

Se recomienda, de hecho, realizar las primeras pruebas con este segundo ejemplo, para trabajar con una solo evento de la señal `SIGCHLD`.

Una vez hecho esto, se debe resolver el problema de que los procesos en _primer plano_ (o `foreground`)
también generan `SIGCHLD` cuando finalizan. El _handler_ los aceptaría, quedando `waitpid(2)`
de `run_cmd()` incapaz de obtener el estado de salida del hijo.

La solución más fácil es asegurarse de que todos los procesos en _segundo plano_ tengan
un mismo _process group_.
Y que la llamada a `waitpid(2)` en el _handler_ no use -1 como argumento
(es decir, esperar por _cualquier proceso_), sino un valor numérico que restrinja
la llamada a los procesos en segundo plano.

**Sugerencia**: configurar el uso de grupos tal que ese primer argumento
de `waitpid(2)` pueda ser, sencillamente, 0.

**Tareas**
  - Manejar los proceso en segundo plano inmediatamente cuando finalizan.
  - Explicar detalladamente el mecanismo completo utilizado.
  - **Responder**:
    - ¿Por qué es necesario el uso de señales?

**Páginas de manual:** `man 7 signal`, `man 7 signal-safety`

**Syscalls sugeridas:** `setpgid(2)`, `getppid(2)`, `sigaction(2)`, `sigaltstack(2)`

**Archivos:** _exec.c_, _runcmd.c_, _sh.c_


## Esqueleto y compilación

**AVISO**: El esqueleto se encuentra disponible en [fisop/shell](https://github.com/fisop/shell).

**IMPORTANTE**: leer el archivo `shell/README.md` que se encuentra en el proyecto. Contiene información sobre cómo realizar la compilación de los archivos, y cómo ejecutar el formateo de código.

Para que no tengan que implementar todo desde cero, se provee un esqueleto. Éste tiene gran parte del parseo hecho, y está estructurado indicando con comentarios los lugares en donde deben introducir el código crítico de cada punto.

Se recomienda, antes de empezar, leer el código para entender bien cómo funciona, y qué hace cada una de las funciones. **Particularmente recomendamos entender qué significa cada uno de los campos en los structs definidos en `types.h`**.

### Compilación

Simplemente alcanza con ejecutar `make`.

### Ejecución

Se proveen dos formas para ejecutar la _shell_: `make run` y `make valgrind` que ejecuta el binario dentro de una sesión de `valgrind`.

### Depurando con printf

Es importante mencionar que es requisito usar las funciones `printf_debug` y `fprintf_debug` si se desea mostrar información por pantalla; o bien encapsular todo lo que se imprima por stdout o stderr utilizando la macro `SHELL_NO_INTERACTIVE` (como ejemplo, ver las funciones definidas en `utils.c`).

Esto es debido a que al momento de corregir es mucho más fácil ejecutar una shell en modo no interactivo (que no imprima _prompt_) y así poder comparar el output de forma automática.

Cualquier mensaje que se imprima por pantalla al momento de hacer la entrega tiene que hacerse con las funciones `printf_debug` (en lugar de `printf`) o bien encapsulando el código con la directiva del preprocesador `#ifndef SHELL_NO_INTERACTIVE`.


## Desafíos

Las tareas listadas aquí no son obligatorias, pero suman para el régimen de [final alternativo](../regimen.md#final).

### Historial

Si bien el _historial_ es también un _built-in_, lo tratamos de forma separada dada su importancia y dificultad técnica

Implementar el _built-in_ `history`, el mismo muestra la lista de comandos ejecutados
hasta el momento. De proporcionarse como argumento `n`, un número entero, solamente
se mostrarán los últimos `n` comandos.

De estar definida la variable de entorno **HISTFILE**, la misma contendrá la ruta al archivo
con los comandos ejecutados en el pasado. En caso contrario, utilizar como ruta por omisión a `~/.fisop_history`.

Permitir navegar el historial de comandos con las teclas &#8593; y &#8595;, de modo tal que se
pueda volver a ejecutar alguno de ellos. Con la tecla &#8593; se accede a un comando anterior,
y con la tecla &#8595; a un comando posterior, si este último no existe, eliminar el comando
actual de modo que solo se vea el prompt.

La tecla BackSpace debe funcionar para borrar los caracteres de un comando de hasta una línea de largo. Al presionar Ctrl&nbsp;+&nbsp;d, la shell debe terminar su ejecución.

**Ayuda**:
 - Para tener mayor control sobre la entrada de caracteres, la terminal debe configurarse en
   modo **no canónico** y **sin** eco. Puede usarse como guía el ejemplo [Noncanonical Mode](https://www.gnu.org/software/libc/manual/html_node/Noncanon-Example.html)
   presente en [Low-Level Terminal Interface](https://www.gnu.org/software/libc/manual/html_node/Low_002dLevel-Terminal-Interface.html).
 - Pueden ser de utilidad algunas [secuencias de escape ANSI](https://en.wikipedia.org/wiki/ANSI_escape_code).
 - Pueden obtener información sobre la terminal con la llamada al sistema `ioctl`, ver: [ioctl_tty(2)](https://man7.org/linux/man-pages/man4/tty_ioctl.4.html).

**Tareas**
  - Soportar el comando `history [n]`
  - Soportar las teclas &#8593; y &#8595;
  - La tecla "backspace" permite borrar comandos de hasta una línea de largo
  - La tecla Ctrl&nbsp;+&nbsp;d debe terminar la ejecución de la shell (como hasta ahora)
  - Soportar la variable de entorno `HISTFILE`
  - Implementar al menos una de las siguientes tareas:
    - Permitir borrar con la tecla BackSpace los caracteres de un comando de cualquier número de líneas, independientemente que la escritura del mismo haya ocasionado el desplazamiento de la pantalla, esto ocurre al continuar escribiendo tras alcanzar la posición inferior derecha de la terminal.
    - Desplazar el cursor de a un caracter con las teclas &#8592; y &#8594;, de a una palabra con Ctrl&nbsp;+&nbsp;&#8592; y Ctrl&nbsp;+&nbsp;&#8594;, al comienzo del comando con Home, y al final del mismo con End, permitiendo insertar nuevos caracteres en cualquier posición.
    - Implementar los designadores de eventos `!!` y `!-n`, ver sección _Event Designators_ en [bash(1)](https://www.man7.org/linux/man-pages/man1/bash.1.html).
  - **Responder**: ¿Cuál es la función de los parámetros `MIN` y `TIME` del modo no canónico? ¿Qué se logra en el ejemplo dado al establecer a `MIN` en `1` y a `TIME` en `0`?
