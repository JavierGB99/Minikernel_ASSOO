/*
 *  usuario/include/servicios.h
 *
 *  Minikernel. Versión 1.0
 *
 *  Fernando Pérez Costoya
 *
 */

/*
 *
 * Fichero de cabecera que contiene los prototipos de funciones de
 * biblioteca que proporcionan la interfaz de llamadas al sistema.
 *
 *      SE DEBE MODIFICAR AL INCLUIR NUEVAS LLAMADAS
 *
 */

#ifndef SERVICIOS_H
#define SERVICIOS_H

/* Evita el uso del printf de la bilioteca estándar */
#define printf escribirf

/* Funcion de biblioteca */
int escribirf(const char *formato, ...);

/* Llamadas al sistema proporcionadas */
int crear_proceso(char *prog);
int terminar_proceso();
int escribir(char *texto, unsigned int longi);
int obtener_id_pr();

//Llamada a la funcion dormir
int dormir (unsigned int segundos);

//Definicion de mutex recursivo o no
#define RECURSIVO 1
#define NO_RECURSIVO 0

//Llamadas de la funcion Mutex
int crear_mutex (char* nombre, int tipo);
int abrir_mutex (char* nombre);
int lock (unsigned int mutex_id);
int unlock (unsigned int mutex_id);
int cerrar_mutex (unsigned int mutex_id);

#endif /* SERVICIOS_H */

