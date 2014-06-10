/*
 *  minikernel/include/kernel.h
 *
 *  Minikernel. Versi�n 1.0
 *
 *  Fernando P�rez Costoya
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
} BCP;

/*
 *
 * Definicion del tipo que corresponde con la cabecera de una lista
 * de BCPs. Este tipo se puede usar para diversas listas (procesos listos,
 * procesos bloqueados en sem�foro, etc.).
 *
 */

typedef struct{
	BCP *primero;
	BCP *ultimo;
} lista_BCPs;

/*
* Variable global que indica el nivel previo de interrupci�n ante 
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
 *
 * Definici�n del tipo que corresponde con una entrada en la tabla de
 * llamadas al sistema.
 *
 */
typedef struct{
	int (*fservicio)();
} servicio;

/*
*
* Definici�n del tipo struct tiempos_ejec
*
*/
struct tiempos_ejec {
	int usuario;
	int sistema;
};


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


/*
 * Variable global que contiene las rutinas que realizan cada llamada
 */
servicio tabla_servicios[NSERVICIOS]={	{sis_crear_proceso},
					{sis_terminar_proceso},
					{sis_escribir}};

#endif /* _KERNEL_H */

