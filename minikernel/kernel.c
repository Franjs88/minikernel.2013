x/*
 *  kernel/kernel.c
 *
 *  Minikernel. Versión 1.0
 *
 *  Fernando Pérez Costoya
 *
 */

/*
 *
 * Fichero que contiene la funcionalidad del sistema operativo
 *
 */

#include "kernel.h"	/* Contiene defs. usadas por este modulo */
#include "string.h"

/*
 *
 * Funciones relacionadas con la tabla de procesos:
 * iniciar_tabla_proc buscar_BCP_libre
 *
 */

/*
 * Función que inicia la tabla de procesos
 */
static void iniciar_tabla_proc(){
	int i;

	for (i=0; i<MAX_PROC; i++)
		tabla_procs[i].estado=NO_USADA;
}

/*
 * Función que busca una entrada libre en la tabla de procesos
 */
static int buscar_BCP_libre(){
	int i;

	for (i=0; i<MAX_PROC; i++)
		if (tabla_procs[i].estado==NO_USADA)
			return i;
	return -1;
}

/*
 *
 * Funciones que facilitan el manejo de las listas de BCPs
 *	insertar_ultimo eliminar_primero eliminar_elem
 *
 * NOTA: PRIMERO SE DEBE LLAMAR A eliminar Y LUEGO A insertar
 */

/*
 * Inserta un BCP al final de la lista.
 */
static void insertar_ultimo(lista_BCPs *lista, BCP * proc){
	if (lista->primero==NULL)
		lista->primero= proc;
	else
		lista->ultimo->siguiente=proc;
	lista->ultimo= proc;
	proc->siguiente=NULL;
}

/*
 * Elimina el primer BCP de la lista.
 */
static void eliminar_primero(lista_BCPs *lista){

	if (lista->ultimo==lista->primero)
		lista->ultimo=NULL;
	lista->primero=lista->primero->siguiente;
}

/*
 * Elimina un determinado BCP de la lista.
 */
static void eliminar_elem(lista_BCPs *lista, BCP * proc){
	BCP *paux=lista->primero;

	if (paux==proc)
		eliminar_primero(lista);
	else {
		for ( ; ((paux) && (paux->siguiente!=proc));
			paux=paux->siguiente);
		if (paux) {
			if (lista->ultimo==paux->siguiente)
				lista->ultimo=paux;
			paux->siguiente=paux->siguiente->siguiente;
		}
	}
}

/*
 *
 * Funciones relacionadas con la planificacion
 *	espera_int planificador
 */

/*
 * Espera a que se produzca una interrupcion
 */
static void espera_int(){
	int nivel;

	printk("-> NO HAY LISTOS. ESPERA INT\n");

	/* Baja al mínimo el nivel de interrupción mientras espera */
	nivel=fijar_nivel_int(NIVEL_1);
	halt();
	fijar_nivel_int(nivel);
}

/*
 * Función de planificacion que implementa un algoritmo FIFO.
 */
static BCP * planificador(){
	while (lista_listos.primero==NULL)
		espera_int();		/* No hay nada que hacer */
	return lista_listos.primero;
}

/*
 *
 * Funcion auxiliar que termina proceso actual liberando sus recursos.
 * Usada por llamada terminar_proceso y por rutinas que tratan excepciones
 *
 */
static void liberar_proceso(){
	BCP * p_proc_anterior;

	liberar_imagen(p_proc_actual->info_mem); /* liberar mapa */

	p_proc_actual->estado=TERMINADO;
	eliminar_primero(&lista_listos); /* proc. fuera de listos */

	/* Realizar cambio de contexto */
	p_proc_anterior=p_proc_actual;
	p_proc_actual=planificador();

	printk("-> C.CONTEXTO POR FIN: de %d a %d\n",
			p_proc_anterior->id, p_proc_actual->id);

	liberar_pila(p_proc_anterior->pila);
	cambio_contexto(NULL, &(p_proc_actual->contexto_regs));
        return; /* no debería llegar aqui */
}

/*
*
* Funciones relacionadas con el uso de MUTEX
*
*/

/*
* Funcion auxiliar que el proceso actual tiene libre algun
* descriptor.
* Return: Posicion si hay disponible
* Return: -1 si error.
*/
int existe_descriptor() {
	int enc = 0;
	int n = 0;
	while((n < NUM_MUT_PROC) && (!enc)) {
		//Comprobamos si tiene descriptor libre
		if(p_proc_actual->descriptores[n].libre == 0) {
			enc = 1;
		}
		n++;
	}
	// Tratamiento de errores en caso de no haber
	if(!enc) {
		return -1;
	}
	else {
		return n-1;
	}

}

/*
* Funcion auxiliar que el proceso actual tiene libre algun
* descriptor.
* Return: Posicion si hay disponible
* Return: -1 si error.
*/
int dame_libre() {
	int enc = 0;
	int n = 0;

	while ((!enc) && (n<NUM_MUT)) {
		//Comprobamos si hay hueco libre
		if(mutex[n].num_procs_en_mutex <= 0) {
			enc = 1;
		}
		n++;
	}
	if(!enc) {
		return LLENO;
	}
	else return n-1;
}

/*
* Funcion auxiliar que verifica si existe ya un MUTEX dado
* Return: 1 si se ha encontrado
* Return: 0 eoc.
*/
int mutex_exist(char*name) {
	int enc = 0;
	int n = 0;

	while((n<NUM_MUT) && (!enc)) {
		//Buscamos en indices que tengan mutex
		if (mutex[n].num_procs_en_mutex > 0) {
			//Verificamos si el nombre coincide
			if(strcmp(name, mutex[n].nombre_mutex)) {
				enc = 1;
			}
		}
		n++;
	}
	if(!enc) {
		return 0;
	}
	else {
		return 1;
	}
}

int dame_posicion_en_mutex(char*nombre) {
	int enc = 0;
	int n = 0;

	while((n<NUM_MUT) && (!enc)) {
		//seleccionamos solo aquellos indices con un mutex
		if (mutex[n].num_procs_en_mutex > 0) {
			// Comparamos solo si coincide el nombre
			if(strcmp(nombre, mutex[n].nombre_mutex) == 0) {
				enc = 1;
			}
		}
		n++;
	}
	if(!enc) {
		return -1;
	}
	else {
		return n-1;
	}
}

/*
 *
 * Funciones relacionadas con el tratamiento de interrupciones
 *	excepciones: exc_arit exc_mem
 *	interrupciones de reloj: int_reloj
 *	interrupciones del terminal: int_terminal
 *	llamadas al sistemas: llam_sis
 *	interrupciones SW: int_sw
 *
 */

/*
 * Tratamiento de excepciones aritmeticas
 */
static void exc_arit(){

	if (!viene_de_modo_usuario())
		panico("excepcion aritmetica cuando estaba dentro del kernel");


	printk("-> EXCEPCION ARITMETICA EN PROC %d\n", p_proc_actual->id);
	liberar_proceso();

        return; /* no debería llegar aqui */
}

/*
 * Tratamiento de excepciones en el acceso a memoria
 */
static void exc_mem(){

	if (!viene_de_modo_usuario())
		panico("excepcion de memoria cuando estaba dentro del kernel");


	printk("-> EXCEPCION DE MEMORIA EN PROC %d\n", p_proc_actual->id);
	liberar_proceso();

        return; /* no debería llegar aqui */
}

/*
 * Tratamiento de interrupciones de terminal
 */
static void int_terminal(){
	char car;

	car = leer_puerto(DIR_TERMINAL);
	printk("-> TRATANDO INT. DE TERMINAL %c\n", car);

        return;
}

/*
 * Tratamiento de interrupciones de reloj
 */
static void int_reloj(){

	printk("-> TRATANDO INT. DE RELOJ\n");

	/* parte asociada a tiempos_proceso */
	num_ints_desde_arranque++;
	// Asignamos recursos de procesador si hay alguno que 
	// lo necesite
	if(lista_listos.primero != NULL) {
		if(viene_de_modo_usuario()){
			p_proc_actual->usuario++;
		}
		else {
			p_proc_actual->sistema++;
		}
        
	}

	/* Tratamos los procesos dormidos */
	if(dormidos.primero != NULL) {
		BCP * pr_dormido;
		BCP * dorm_aux;

		pr_dormido = dormidos.primero;
		while(pr_dormido != NULL) {
			pr_dormido->segs--;
			// Si algun proceso terminado su espera
			if(pr_dormido->segs <= 0) {
				//reajustamos listas
				pr_dormido->estado = LISTO;

				if(pr_dormido->siguiente != NULL) {
					// El puntero interno se modifica
					// Guardamos una copia antes para mantener el orden
					dorm_aux = pr_dormido->siguiente;
					eliminar_elem(&dormidos, pr_dormido);
					insertar_ultimo(&lista_listos, pr_dormido);
					pr_dormido = dorm_aux;
				}
				else {
					eliminar_elem(&dormidos, pr_dormido);
					insertar_ultimo(&lista_listos, pr_dormido);
				}

				pr_dormido = pr_dormido->siguiente;

			}
		}
	}

	/* Si hay algun proceso en estado de listo lo tratamos */
	if(lista_listos.primero != NULL) {

	}

	return;
}

/*
 * Tratamiento de llamadas al sistema
 */
static void tratar_llamsis(){
	int nserv, res;

	nserv=leer_registro(0);
	if (nserv<NSERVICIOS)
		res=(tabla_servicios[nserv].fservicio)();
	else
		res=-1;		/* servicio no existente */
	escribir_registro(0,res);
	return;
}

/*
 * Tratamiento de interrupciuones software
 */
static void int_sw(){

	printk("-> TRATANDO INT. SW\n");

	return;
}

/*
 *
 * Funcion auxiliar que crea un proceso reservando sus recursos.
 * Usada por llamada crear_proceso.
 *
 */
static int crear_tarea(char *prog){
	void * imagen, *pc_inicial;
	int error=0;
	int proc;
	BCP *p_proc;

	proc=buscar_BCP_libre();
	if (proc==-1)
		return -1;	/* no hay entrada libre */

	/* A rellenar el BCP ... */
	p_proc=&(tabla_procs[proc]);

	/* crea la imagen de memoria leyendo ejecutable */
	imagen=crear_imagen(prog, &pc_inicial);
	if (imagen)
	{
		p_proc->info_mem=imagen;
		p_proc->pila=crear_pila(TAM_PILA);
		fijar_contexto_ini(p_proc->info_mem, p_proc->pila, TAM_PILA,
			pc_inicial,
			&(p_proc->contexto_regs));
		p_proc->id=proc;
		p_proc->estado=LISTO;

		/* lo inserta al final de cola de listos */
		insertar_ultimo(&lista_listos, p_proc);
		error= 0;
	}
	else
		error= -1; /* fallo al crear imagen */

	return error;
}

/*
 *
 * Rutinas que llevan a cabo las llamadas al sistema
 *	sis_crear_proceso sis_escribir
 *obtener
 */

 /*
 * Tratamiento de la llamada al sistema obtener_id_pr.
 *
 */
 int sis_obtener_id_pr() {
 	return p_proc_actual->id;
 }

/*
 * Tratamiento de la llamada al sistema dormir.
 *
 */
 int sis_dormir() {
 	BCP * p_proc_anterior;
 	// Ponemos el estado a bloqueado y
 	// leemos el num de segs del registro 1
 	p_proc_actual->estado = BLOQUEADO;
 	p_proc_actual->segs = leer_registro(1)*TICK;
 	// indicamos que ya no es necesario realizar
 	// cambio de contexto involuntario
 	p_proc_actual->replanificacion = 0;
 	nivel_previo = fijar_nivel_int(NIVEL_3);
 	// Eliminamos de la lista de procesos listos 
 	// e insertamos en la lista de dormidos
 	eliminar_primero(&lista_listos);
 	insertar_ultimo(&dormidos, p_proc_actual);
 	// hacemos un cambio de contexto
 	p_proc_anterior = p_proc_actual;
 	p_proc_actual = planificador();

 	printk("*** CAMBIO CONTEXTO DORMIR: de %d hasta %d\n",
 		p_proc_anterior->id, p_proc_actual->id);

 	// Restauramos el contexto de nuestro nuevo proc_actual
 	cambio_contexto(&(p_proc_anterior->contexto_regs),
 	 &(p_proc_actual->contexto_regs));
 	//fijamos nivel previo de interrupciones
 	fijar_nivel_int(nivel_previo);
 	return 0;
 } 

 /*
 * Tratamiento de la llamada al sistema tiempos_proceso
 *
 */
 int sis_tiempos_proceso() {
 	struct tiempos_ejec *t_ejec;
 	t_ejec = (struct tiempos_ejec *)leer_registro(1);
 	
 	if(t_ejec != NULL ) {
 		nivel_previo = fijar_nivel_int(3);
 		t_ejec->usuario = p_proc_actual->usuario;
 		t_ejec->sistema = p_proc_actual->sistema;
 		accede = 1;
 		fijar_nivel_int(nivel_previo);
 	}
 	return num_ints_desde_arranque;
 }


 /*
 * Comienza la parte de MUTEX
 */

 /*
 *	Tratamiento de la llamada al sistema crear_mutex. Llama
 *  a las funciones auxiliares mutex_exist, existe_descriptor y 
 *	dame_libre.
 */
 int sis_crear_mutex() {
 	BCP * p_proc_anterior;
 	char*nombre = (char*)leer_registro(1);
 	int pos;
 	int exists;
 	int disponibilidad;
 	int type = leer_registro(2);

 	// Vemos si hay descriptor libres
 	pos = existe_descriptor();

 	// Captura de error en caso de no haberlo
 	if(pos == -1){
 		printk("ERROR: Ya existe ese MUTEX");
 		return -1;
 	}

 	// en cualquier otro caso:
 	exists = mutex_exist(nombre);

 	if(exists) {
 		printk("ERROR: ya existe el MUTEX");
 		return -1;
 	}

 	// Ahora comprobamos si hay algun espacio en el sistema
 	disponibilidad = dame_libre();

 	// Si no hay huecos, se bloquea
 	while(disponibilidad == LLENO) {
 		p_proc_actual->estado = BLOQUEADO;
 		p_proc_actual->replanificacion = 0;
 		nivel_previo = fijar_nivel_int(NIVEL_3);
 		eliminar_primero(&lista_listos);
 		// Lo insertamos al final de la lista
 		insertar_ultimo(&lista_de_mutex, p_proc_actual);
 		// Hacemos un c. de contexto
 		p_proc_anterior = p_proc_actual;
 		//Esperamos a que haya un proceso listo
 		p_proc_actual = planificador();
 		//Imprimimos:
 		printk("*** CAMBIO CONTEXTO POR FUNCION CREAR MUTEX: de %d a %d\n",
 			p_proc_anterior->id, p_proc_actual->id);
 		// Restauramos contexto del nuevo proceso actual
 		cambio_contexto(&(p_proc_anterior->contexto_regs),
 			&(p_proc_actual->contexto_regs));
 		fijar_nivel_int(nivel_previo);
 		disponibilidad = dame_libre();
 	}
 	// En cualquier otro caso comprobamos:
 	// Si existe ya un mutex con dicho nombre
 	exists = mutex_exist(nombre);

 	if(exists) {
 		printk("ERROR: ya existe el mutex");
 		return -1;
 	}

 	//Tras las verificaciones
 	//Creamos el MUTEX:
 	mutex[disponibilidad].propietario = p_proc_actual->id;
 	mutex[disponibilidad].num_procs_en_mutex++;
 	mutex[disponibilidad].tipo = type;
 	mutex[disponibilidad].nombre_mutex = strdup(nombre);

 	p_proc_actual->descriptores[pos].descript = disponibilidad;
 	p_proc_actual->descriptores[pos].libre = 1;
 	return disponibilidad;

 }

/*
 *	Tratamiento de llamada al sistema abrir_mutex. Llama a las funciones
 *	auxiliares 
 */
int sis_abrir_mutex() {
	char*nombre = (char*)leer_registro(1);
 	int pos;
 	int exists;
 	int descriptor;

 	pos = existe_descriptor();
 	//Si no hay descriptor:
 	if(pos == -1) {
 		printk("ERROR: no hay descriptores libres para el proceso %d\n",
 			p_proc_actual->id);
 		return -1;
 	}

 	// Si no se cumple lo anterior:
 	exists = mutex_exist(nombre);
 	//Si no existe:
 	if (!exists) {
 		printk("ERROR: no existe el MUTEX en el sistema operativo\n");
 		return -1;
 	}

 	// Si hemos llegado hasta aqui se han cumplido las precondiciones
 	// Por lo que concedemos el descriptor al mutex
 	descriptor = dame_posicion_en_mutex(nombre);
 	mutex[descriptor].num_procs_en_mutex++;
 	p_proc_actual->descriptores[pos].descript = descriptor;
 	p_proc_actual->descriptores[pos].libre = 1;

 	return descriptor;
}

int sis_lock() {
	BCP*p_proc_anterior;
	int blocked;
	unsigned int mutexid = (unsigned int)leer_registro(1);

	do {
		blocked = 0;
		if(mutex[mutexid].num_procs_en_mutex > 0) {
			//Verificamos si esta o no esta bloqueado
			if(mutex[mutexid].bloqueado > 0) {
				//Comprobamos si es RECURSIVO
				if(mutex[mutexid].tipo == RECURSIVO) {
					//Comprobamos si es el dueño
					if(mutex[mutexid].propietario == p_proc_actual->id) {
						//Aumentamos el numero de bloqueos en el mutex
						mutex[mutexid].bloqueado++;
					}
					// Si no, bloqueamos al proceso
					else {
						p_proc_actual->estado = BLOQUEADO;
						// Ya no se debe hacer C. de contexto involuntario
						p_proc_actual->replanificacion = 0;
						nivel_previo = fijar_nivel_int(NIVEL_3);

						eliminar_primero(&lista_listos);
						//Lo insertamos en la lista de bloqueados por un lock
						insertar_ultimo(&lista_de_bloqueados, p_proc_actual);
						//Hacemos un C de Contexto
						p_proc_anterior = p_proc_actual;
						p_proc_actual = planificador();

						printk("*** C de CONTEXTO POR UN LOCK: de %d a %d\n",
						p_proc_anterior->id, p_proc_actual->id);
						
						//Restauramos el contexto del nuevo actual
						cambio_contexto(&(p_proc_anterior->contexto_regs), &(p_proc_actual->contexto_regs));
						fijar_nivel_int(nivel_previo);
						//Ahora indicamos que hay que volver a comprobar para que no se 
						//cuele ningun proceso
						blocked = 1;
					}
				}
				else if(mutex[mutexid].tipo == NO_RECURSIVO) {
					//Vemos si es el dueño del bloqueo
					if(mutex[mutexid].propietario == p_proc_actual->id) {
						//Comprobamos si ya hay un proceso bloqueandolo
						if(mutex[mutexid].propietario == p_proc_actual->id) {
							//Si es asi, capturamos el error. Ya que se produciria interbloqueo
							printk("ERROR: se esta produciendo un caso de interbloqueo trivial\n");
							return -1;
						}
						//En caso contrario bloqueamos el mutex
						mutex[mutexid].bloqueado++;
					}
					//Si no es el dueño bloqueamos al proceso
					else {
						p_proc_actual->estado = BLOQUEADO;
						// Ya no es necesario hacer cambio de contexto involuntario
						p_proc_actual->replanificacion = 0;
						nivel_previo = fijar_nivel_int(NIVEL_3);
						
						eliminar_primero(&lista_listos);
						insertar_ultimo(&lista_de_bloqueados, p_proc_actual);
						//Hacemos un C de Contexto
						p_proc_anterior = p_proc_actual;
						p_proc_actual = planificador();

						printk("*** C de CONTEXTO POR UN LOCK: de %d a %d\n",
							p_proc_anterior->id, p_proc_actual->id);
						//Restauramos el contexto del nuevo actual
						cambio_contexto(&(p_proc_anterior->contexto_regs),
							&(p_proc_actual->contexto_regs));
						fijar_nivel_int(nivel_previo);

						//Indamos que hay que volver a comprobar para que no se cuele
						//ningun proceso
						blocked = 1;
					}
				}
			}
			else if(mutex[mutexid].bloqueado == 0) {
				//Hacemos que el proceso actual pase a ser el nuevo propietario
				//Bloqueamos al mutex
				mutex[mutexid].bloqueado++;
				mutex[mutexid].propietario = p_proc_actual->id;
			}
			else {
				printk("ERROR: error interno en el mutex");
				return -1;
			}
		}
		else {
			printk("ERROR: se esta intentando bloquar un mutex que aun no ha sido abierto");
			return -1;
		}
	}
	while (blocked);
	return 0;
}

int sis_unlock() {
	return 0;
}

/*
 * Tratamiento de llamada al sistema crear_proceso. Llama a la
 * funcion auxiliar crear_tarea sis_terminar_proceso
 */
int sis_crear_proceso(){
	char *prog;
	int res;

	printk("-> PROC %d: CREAR PROCESO\n", p_proc_actual->id);
	prog=(char *)leer_registro(1);
	res=crear_tarea(prog);
	return res;
}

/*
 * Tratamiento de llamada al sistema escribir. Llama simplemente a la
 * funcion de apoyo escribir_ker
 */
int sis_escribir()
{
	char *texto;
	unsigned int longi;

	texto=(char *)leer_registro(1);
	longi=(unsigned int)leer_registro(2);

	escribir_ker(texto, longi);
	return 0;
}

/*
 * Tratamiento de llamada al sistema terminar_proceso. Llama a la
 * funcion auxiliar liberar_proceso
 */
int sis_terminar_proceso(){

	printk("-> FIN PROCESO %d\n", p_proc_actual->id);

	liberar_proceso();

        return 0; /* no debería llegar aqui */
}

/*
 *
 * Rutina de inicialización invocada en arranque
 *
 */
int main(){
	/* se llega con las interrupciones prohibidas */

	instal_man_int(EXC_ARITM, exc_arit); 
	instal_man_int(EXC_MEM, exc_mem); 
	instal_man_int(INT_RELOJ, int_reloj); 
	instal_man_int(INT_TERMINAL, int_terminal); 
	instal_man_int(LLAM_SIS, tratar_llamsis); 
	instal_man_int(INT_SW, int_sw); 

	iniciar_cont_int();		/* inicia cont. interr. */
	iniciar_cont_reloj(TICK);	/* fija frecuencia del reloj */
	iniciar_cont_teclado();		/* inici cont. teclado */

	iniciar_tabla_proc();		/* inicia BCPs de tabla de procesos */

	/* crea proceso inicial */
	if (crear_tarea((void *)"init")<0)
		panico("no encontrado el proceso inicial");
	
	/* activa proceso inicial */
	p_proc_actual=planificador();
	cambio_contexto(NULL, &(p_proc_actual->contexto_regs));
	panico("S.O. reactivado inesperadamente");
	return 0;
}
