/*
 *  minikernel/include/HAL.h
 *
 *  Minikernel. Versi�n 1.0
 *
 *  Fernando P�rez Costoya
 *
 */

/*
 *
 * Fichero de cabecera que contiene los prototipos de las funciones
 * proporcionadas por el m�dulo HAL.
 *
 * 	NO SE DEBE MODIFICAR
 *
 */

#ifndef _HAL_H
#define _HAL_H


/* La versi�n actual no usa en Linux las primitivas de gesti�n de contexto
   disponibles en algunos UNIX ("makecontext", "setcontext", ...), ya que,
   que yo sepa, por ahora no est�n implementadas. En su lugar recurre
   a una versi�n m�s "cutre" basada en "setjmp". n una pr�xima versi�n
   deberian desaparecer. */

#ifndef __linux__

/* Para sistemas con "makecontext" como Digital UNIX */
#include <ucontext.h>

#else /* linux */
#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,1)
/* Para Linux modernos que tambi�n lo tienen */
#include <ucontext.h>

#else
/* Para Linux antiguos */
#include <setjmp.h>
#include <signal.h>

typedef struct st_contexto {
	stack_t uc_stack;
	char * args;
	sigjmp_buf contexto;
	void * uc_link;
	sigset_t uc_sigmask;
	
} ucontext_t;

#endif /* linux2.2 */
#endif /* linux */

/* Registros generales del procesador */
#define NREGS 6

/* Contexto "hardware" de un proceso */
typedef struct {
	ucontext_t ctxt;
	long registros[NREGS];
} contexto_t;



/*
 *
 * Operaciones relacionadas con los dispositivos y las interrupciones.
 *
 */

unsigned long long int leer_reloj_CMOS(); /* obtiene hora del rejoj CMOS */

void iniciar_cont_reloj(int ticks_por_seg); /* iniciar controlador de reloj */

void iniciar_cont_teclado(); /* iniciar controlador de teclado */

void iniciar_cont_int();  /* iniciar controlador de interrupciones. */

void instal_man_int(int nvector, void (*manej)()); /* instala un manejador */

int fijar_nivel_int(int nivel); /* fija nivel de interrupci�n
                                   del procesador devolviendo el previo */

int viene_de_modo_usuario(); /* Devuelve verdadero si el modo previo de
			    ejecuci�n del procesador era usuario */

void activar_int_SW(); /* activa la interrupci�n SW */

/*
 *
 * Operaci�n de salvaguarda y recuperaci�n de contexto hardware del proceso.
 * Rutina que realiza el cambio de contexto. Si (contexto_a_salvar==NULL)
 * no salva contexto, s�lo restaura
 *
 */
void cambio_contexto(contexto_t *contexto_a_salvar, contexto_t *contexto_a_restaurar);


/*
 *
 * Operaciones relacionadas con mapa de memoria del proceso y pila
 *
 */

/* crea mapa de memoria a partir de ejecutable "prog" devolviendo un
descriptor de dicho mapa y la direcci�n del punto de arranque del programa */
void * crear_imagen(char *prog, void **dir_ini); 

void * crear_pila(int tam); /* crea la pila del proceso */

/* crea el contexto inicial del proceso */
void fijar_contexto_ini(void *mem, void *p_pila, int tam_pila,
			void * pc_inicial, contexto_t *contexto_ini);

void liberar_imagen(void *mem);		/* libera el mapa de memoria */

void liberar_pila(void *pila);		/* libera la pila del proceso */

/*
 *
 * Operaciones miscel�neas
 *
 */

long leer_registro(int nreg); 

int escribir_registro(int nreg, long valor); 

char leer_puerto(int dir_puerto); /* lee un car�cter del puerto especificado */

void halt();	/* Ejecuta una instrucci�n HALT para parar UCP */

void panico(char *mens); /* muestra mensaje y termina SO */

void escribir_ker(char *buffer, unsigned int longi); /* escribe en pantalla */

#define printf printk /* evita uso de printf de bilioteca est�ndar */

int printk(const char *, ...); /* escribe en pantalla con formato */


#endif /* _HAL_H */
