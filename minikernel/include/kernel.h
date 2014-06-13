/*
 *  minikernel/include/kernel.h
 *
 *  Minikernel. Versión 1.0
 *
 *  Fernando Pérez Costoya
 *
 */

/*
 *
 * Fichero de cabecera que contiene definiciones usadas por kernel.c
 *
 *      SE DEBE MODIFICAR PARA INCLUIR NUEVA FUNCIONALIDAD
 *
 */

#ifndef _KERNEL_H
#define _KERNEL_H

#include "const.h"
#include "HAL.h"
#include "llamsis.h"


/*
*	Definicion del tipo para los descriptores de proceso
*/
typedef struct {
	int descript;
	int libre;	
} tipo_descriptor;


/*
 *
 * Definicion del tipo que corresponde con el BCP.
 * Se va a modificar al incluir la funcionalidad pedida.
 *
 */
typedef struct BCP_t *BCPptr;

typedef struct BCP_t {
        int id;				/* ident. del proceso */
        int estado;			/* TERMINADO|LISTO|EJECUCION|BLOQUEADO*/
        contexto_t contexto_regs;	/* copia de regs. de UCP */
        void * pila;			/* dir. inicial de la pila */
	BCPptr siguiente;		/* puntero a otro BCP */
	void *info_mem;			/* descriptor del mapa de memoria */
	unsigned int segs;		/* segundos que permance dormido el proceso*/
	int replanificacion;	/* booleano para saber cuando hay que hacer un
							 cambio de contexto involuntario */
	int sistema;			/* Indica el numero de ticks que proc ejecuta en modo sistema*/
	int usuario;			/* Indica el numero de ticks que proc ejecuta en modo usuario*/
	tipo_descriptor descriptores[NUM_MUT_PROC];

} BCP;


/*
 *
 * Definicion del tipo que corresponde con la cabecera de una lista
 * de BCPs. Este tipo se puede usar para diversas listas (procesos listos,
 * procesos bloqueados en semáforo, etc.).
 *
 */

typedef struct{
	BCP *primero;
	BCP *ultimo;
} lista_BCPs;


/*
*
* Definición del tipo struct tiempos_ejec
*
*/
struct tiempos_ejec {
	int usuario;
	int sistema;
};


/*
* Variable global que indica el nivel previo de interrupción ante 
* un cambio en el nivel de interrupcion
*/
int nivel_previo;

/*
* Variable global que indica el numero de interrupciones de reloj 
* producidas desde el arranque del sistema
*/
int num_ints_desde_arranque;

/*
* Variable global que indica que se esta accediendo en modo sistema
* a la zona donde referencia esta variable
*/
int accede = 0;


/*
 * Variable global que identifica el proceso actual
 */

BCP * p_proc_actual=NULL;

/*
 * Variable global que representa la tabla de procesos
 */

BCP tabla_procs[MAX_PROC];

/*
 * Variable global que representa la cola de procesos listos
 */
lista_BCPs lista_listos= {NULL, NULL};

/*
* Variable gloal que representa la lista de procesos dormidos
*/
lista_BCPs dormidos = {NULL, NULL};

/*
* Variable global que representa la lista de procesos en cola bloqueados
* al crear el mutex
*/
lista_BCPs lista_de_mutex = {NULL, NULL};

/*
* Variable global que representa la lista de procesos en cola bloqueados
* en estado lock y unlock
*/
lista_BCPs lista_de_bloqueados = {NULL, NULL};

/*
 *
 * Definición del tipo que corresponde con una entrada en la tabla de
 * llamadas al sistema.
 *
 */
typedef struct{
	int (*fservicio)();
} servicio;


// Especificacion del MUTEX

#define NO_RECURSIVO 0
#define RECURSIVO 1

typedef struct {
	int propietario;	//Muestra que proceso es dueno del mutex
	int num_procs_en_mutex;	// Indica numero de procesos en el mutex
	int tipo;	// Indica de que tipo es: RECURSIVO O NO_RECURSIVO
	int bloqueado;	/* Positivo->Indica numero de veces que se ha bloqueado
					*  = 0 -> Indica que el mutex esta libre
					*  Negativo -> Indica un error
					*/
	char*nombre_mutex;
} tipo_mutex;

tipo_mutex mutex[NUM_MUT];


/*
* Variable global que indica el tamano del buffer
* de caracteres leidos.
*/
#define VACIO -2
#define LLENO -1

typedef struct {
	int character;
} tipo_buffer;

tipo_buffer buffer[TAM_BUF_TERM];


/*
 * Prototipos de las rutinas que realizan cada llamada al sistema
 */
int sis_crear_proceso();
int sis_terminar_proceso();
int sis_escribir();

/* Funcionalidad adicional */
int sis_obtener_id_pr();
int sis_dormir();
int sis_tiempos_proceso();
int sis_crear_mutex();
int sis_abrir_mutex();
int sis_lock();
int sis_unlock();
int sis_cerrar_mutex();
//int sis_leer_caracter();


/*
 * Variable global que contiene las rutinas que realizan cada llamada
 */
servicio tabla_servicios[NSERVICIOS]={	{sis_crear_proceso},
					{sis_terminar_proceso},
					{sis_escribir},
					{sis_obtener_id_pr},
					{sis_dormir},
					{sis_tiempos_proceso},
					{sis_crear_mutex},
					{sis_abrir_mutex},
					{sis_lock},
					{sis_unlock},
					{sis_cerrar_mutex}};//,
					//{sis_leer_caracter}};

#endif /* _KERNEL_H */

