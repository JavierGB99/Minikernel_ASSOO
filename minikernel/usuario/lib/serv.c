/*
 *  usuario/lib/serv.c
 *
 *  Minikernel. Versión 1.0
 *
 *  Fernando Pérez Costoya
 *
 */

/*
 *
 * Fichero que contiene las definiciones de las funciones de interfaz
 * a las llamadas al sistema. Usa la funcion de apoyo llamsis
 *
 *      SE DEBE MODIFICAR AL INCLUIR NUEVAS LLAMADAS
 *
 */

#include "llamsis.h"
#include "servicios.h"

/* Función del módulo "misc" que prepara el código de la llamada
   (en el registro 0), los parámetros (en registros 1, 2, ...), realiza la
   instruccion de llamada al sistema  y devuelve el resultado 
   (que obtiene del registro 0) */

int llamsis(int llamada, int nargs, ... /* args */);


/*
 *
 * Funciones interfaz a las llamadas al sistema
 *
 */


int crear_proceso(char *prog){
	return llamsis(CREAR_PROCESO, 1, (long)prog);
}
int terminar_proceso(){
	return llamsis(TERMINAR_PROCESO, 0);
}
int escribir(char *texto, unsigned int longi){
	return llamsis(ESCRIBIR, 2, (long)texto, (long)longi);
}
int obtener_id_pr(){
	return llamsis(OBTENERID, 3);
}
int dormir (unsigned int segundos) {
	return llamsis(DORMIR, 4, (long) segundos);
}
int crear_mutex (char* nombre, int tipo) {
	return llamsis(CREAR_MUTEX, 5, (long) nombre,(long) tipo);
}
int abrir_mutex (char* nombre) {
	return llamsis(ABRIR_MUTEX, 6, (long) nombre);
}
int lock (unsigned int mutex_id) {
	return llamsis(LOCK, 7, (long) mutex_id);
}
int unlock (unsigned int mutex_id) {
	return llamsis(UNLOCK, 8, (long) mutex_id);
}
int cerrar_mutex (unsigned int mutex_id) {
	return llamsis(CERRAR_MUTEX, 9, (long) mutex_id);
}
