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

#define NO_RECURSIVO 0
#define RECURSIVO 1

#include "const.h"
#include "HAL.h"
#include "llamsis.h"
#include <string.h>

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
				int t_dormir; // Guarda el tiempo que se duerme
				int n_descriptores; //3Numero de descriptores
				int descriptores[NUM_MUT_PROC]; //3) AÑADIDA. Array de descriptores
		//4) Variables round robin
				unsigned int slice; /*Tiempo de ejecucion que le queda al proceso(Rodaja) !!!!!!!!*/
				BCPptr siguiente;		/* puntero a otro BCP */
				void *info_mem;			/* descriptor del mapa de memoria */

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
 *
 * Definici�n del tipo que corresponde con una entrada en la tabla de
 * llamadas al sistema.
 *
 */
typedef struct{
	int (*fservicio)();
} servicio;



/*
 * Prototipos de las rutinas que realizan cada llamada al sistema
 */
int sis_crear_proceso();
int sis_terminar_proceso();
int sis_escribir();
int obtener_id_pr();

//Dormir
int dormir(unsigned int segundos);//llamada de dormir

lista_BCPs lista_dormidos = {NULL, NULL};

//MUTEX
int crear_mutex (char *nombre, int tipo);
int abrir_mutex(char *nombre);
int cerrar_mutex (unsigned int mutexid);
int lock (unsigned int mutexid);
int unlock (unsigned int mutexid);

//Struct para el mutex
typedef struct {
	char *nombre;
	int tipo; //Recursivo o no recursivo
	int propietario; //Id del proceso
	int abierto; 
	int locked;
	//Lista de cada mutex
	lista_BCPs lista_proc_esperando_lock;
} mutex;
//Array para guardar mutex utilizados. Tamaño = numero maximo de mutex
mutex array_mutex[NUM_MUT];
//Variable que almacena el numero de mutex creados
int mutex_creados;
//Lista de procesos bloqueados porque se habian creado el numero maximo de mutex permitidos
lista_BCPs lista_bloq_mutex = {NULL, NULL};

/*
 * Variable global que contiene las rutinas que realizan cada llamada
 */
servicio tabla_servicios[NSERVICIOS]={	
					{sis_crear_proceso},
					{sis_terminar_proceso},
					{sis_escribir},
					{obtener_id_pr}, 
					{dormir}, //1) Rutina Dormir
					{crear_mutex},
					{abrir_mutex},
					{lock},
					{unlock},
					{cerrar_mutex}
				};

#endif /* _KERNEL_H */

