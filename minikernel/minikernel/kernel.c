/*
 *  kernel/kernel.c
 *
 *  Minikernel. Versi�n 1.0
 *
 *  Fernando P�rez Costoya
 *
 */

/*
 *
 * Fichero que contiene la funcionalidad del sistema operativo
 *
 */

#include "kernel.h"	/* Contiene defs. usadas por este modulo */
#include <string.h>

/*
 *
 * Funciones relacionadas con la tabla de procesos:
 *	iniciar_tabla_proc buscar_BCP_libre
 *
 */

/*
 * Funci�n que inicia la tabla de procesos
 */
static void iniciar_tabla_proc(){
	int i;

	for (i=0; i<MAX_PROC; i++)
		tabla_procs[i].estado=NO_USADA;
}

/*
 * Funci�n que busca una entrada libre en la tabla de procesos
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

	/* Baja al m�nimo el nivel de interrupci�n mientras espera */
	nivel=fijar_nivel_int(NIVEL_1);
	halt();
	fijar_nivel_int(nivel);
}

/*
 * Funci�n de planificacion que implementa un algoritmo FIFO.
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
        return; /* no deber�a llegar aqui */
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

        return; /* no deber�a llegar aqui */
}

/*
 * Tratamiento de excepciones en el acceso a memoria
 */
static void exc_mem(){

	if (!viene_de_modo_usuario())
		panico("excepcion de memoria cuando estaba dentro del kernel");


	printk("-> EXCEPCION DE MEMORIA EN PROC %d\n", p_proc_actual->id);
	liberar_proceso();

        return; /* no deber�a llegar aqui */
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


	BCP* proceso = lista_dormidos.primero;
		
	while (proceso != NULL) {
		//Disminuir su tiempo
		proceso->t_dormir--;
		printk("El proceso cuyo id es %d le quedan %d\n", proceso-> id, proceso->t_dormir);
		//Si el plazo ha acabado, desbloquear
		//Antes de borrarlo hay que guardar el siguiente proceso al que apunta en la cola dormidos
		//Si se borra despues, apunta a otro que no esta dormido!!
		BCP* proceso_siguiente = proceso->siguiente;
		if(proceso->t_dormir <= 0) {
			//Elevar nivel interrupcion y guardar actual (desconocido)
			int nivel_int = fijar_nivel_int(NIVEL_3); //Inhibir int reloj mientras se manejan listas
			
			printk("El proceso con id = %d despierta\n", proceso->id);
			//Cambiar el estado
			proceso->estado = LISTO;
			//Quitarlo de la lista dormidos
			eliminar_elem(&lista_dormidos, proceso);
			//Ponerlo el ultimo en la cola de listos
			insertar_ultimo(&lista_listos, proceso);

			//Volver al nivel de interrupcion anterior
			fijar_nivel_int(nivel_int);		
		}
		//Pasar al siguiente elemento en la cola de dormidos
		proceso = proceso_siguiente;
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
 *
 */

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

	//3) Antes de liberar proceso hay que cerrar todos sus mutex
	if (p_proc_actual->descriptores > 0) {
		printk("Cerrar mutex que ha abierto el proceso\n");
		for(int i = 0; i < NUM_MUT_PROC; i++) {
			printk("%d\n", i);
			cerrar_mutex(i);			
		}
		printk("se ha terminado de cerrar los mutex\n");
	}

	liberar_proceso();

        return 0; /* no deber�a llegar aqui */
}

int obtener_id_pr(){
	int id = p_proc_actual->id;
	printk("ID del proceso actual es: %d\n", id);
	return id;
}

int dormir (unsigned int segundos){

	int nivel_int;	
	segundos = (unsigned int)leer_registro(1);
	
	//Guardamos interrupcion 
	nivel_int = fijar_nivel_int(NIVEL_3);	

	//Actualizamor BCP
	p_proc_actual->estado = BLOQUEADO;
	p_proc_actual->t_dormir = segundos*TICK;
	BCP* p_proc_dormido = p_proc_actual; 



	// Sacar de la lista
	eliminar_primero(&lista_listos);
	// Insertarlo
	insertar_ultimo(&lista_dormidos, p_proc_dormido);

	// Restaurar interrupcion
	fijar_nivel_int(nivel_int);


	//cambio de contexto

	p_proc_actual=planificador();
	cambio_contexto(&(p_proc_dormido->contexto_regs), &(p_proc_actual->contexto_regs));

	return 0;
}

///////////////////////////
///////////////////////////
///////////////////////////
int descriptor_libre () {
	
	int aux = -1;
	int i = 0;

	while((aux == -1) && (i <= NUM_MUT_PROC)) {
		//Si descriptor = -1, no ha sido utilizado
		if(p_proc_actual->descriptores[i] == -1) {
			aux = i;
		}
		
		i++;
	}

	return aux;		
}

int crear_mutex (char *nombre, int tipo){
	printk("Creando mutex\n");
	nombre=(char *)leer_registro(1);
	tipo=(int)leer_registro(2);

	if(strlen(nombre) > MAX_NOM_MUT){
		return -1;
	}

	
	int i;
	for (i = 0; i < NUM_MUT; i++){
		if(array_mutex[i].nombre != NULL && 
			strcmp(array_mutex[i].nombre, nombre) == 0){
			return -1;
		}
	}

	int descriptor_proc = descriptor_libre();
	if(descriptor_proc == -1) {
		printk("El proceso no tiene descriptores libres\n");
		return -1; 
	}

	
	while(mutex_creados==NUM_MUT) {

		int nivel_int = fijar_nivel_int(NIVEL_3);

		p_proc_actual->estado=BLOQUEADO;

		eliminar_primero(&lista_listos);
		insertar_ultimo(&lista_bloq_mutex, p_proc_actual);

		BCP* p_proc_bloq = p_proc_actual;
		p_proc_actual=planificador();

		cambio_contexto(&(p_proc_bloq->contexto_regs), &(p_proc_actual->contexto_regs));
		fijar_nivel_int(nivel_int);
	}
	

	int descriptor_mut = descriptor_mutex();


	p_proc_actual->descriptores[descriptor_proc] = descriptor_mut;

	array_mutex[descriptor_mut].nombre = nombre;
	array_mutex[descriptor_mut].tipo = tipo;
	array_mutex[descriptor_mut].abierto++;
	mutex_creados++;
	p_proc_actual->n_descriptores++;

	printk("Mutex creado correctamente\n");
	return descriptor_proc; 
}



int abrir_mutex(char *nombre) {
	printk("Abriendo mutex\n");
	nombre=(char *)leer_registro(1);

	if(strlen(nombre) > MAX_NOM_MUT){
		return -1;
	}

	

	if((nombres_iguales(nombre)) == 0) {
		printk("No se ha encontrado un mutex con este nombre\n");
		return -1;
	}

	int descriptor_proc = descriptor_libre();
	if(descriptor_proc == -1) {
		printk("El proceso no tiene descriptores libres\n");
		return -1; 
	}

	
	int descriptor_mut;
	for(int i=0; i<mutex_creados; i++) {
		if(strcmp(array_mutex[i].nombre, nombre) == 0) {
			descriptor_mut = i;
		}
	}


	p_proc_actual->descriptores[descriptor_proc]=descriptor_mut;
	p_proc_actual->n_descriptores++;

	array_mutex[descriptor_mut].abierto++;

	printk("Mutex abierto correctamente\n");
	return descriptor_proc; 
}




int lock (unsigned int mutexid) {
	printk("Haciendo lock\n");
	int desc_proc=(unsigned int)leer_registro(1); 
	mutexid = p_proc_actual->descriptores[desc_proc];
	int proceso_esperando = 1; 

	if(mutexid == -1) {
		printk("El mutex no existe \n");
		return -1;
	}

	while (proceso_esperando) {

		
		if(array_mutex[mutexid].locked > 0) {
			if((array_mutex[mutexid].tipo) == RECURSIVO) {
				if(array_mutex[mutexid].propietario == p_proc_actual->id) {
					
					array_mutex[mutexid].locked++;
					proceso_esperando = 0;
				}
				
				else {
					int nivel_int = fijar_nivel_int(NIVEL_3);

					
					p_proc_actual->estado=BLOQUEADO;
					
					eliminar_primero(&lista_listos);
					insertar_ultimo(&(array_mutex[mutexid].lista_proc_esperando_lock), p_proc_actual);
					
					
					BCP* p_proc_bloq = p_proc_actual; 
					p_proc_actual=planificador();
					
					
					cambio_contexto(&(p_proc_bloq->contexto_regs), &(p_proc_actual->contexto_regs));

					
					fijar_nivel_int(nivel_int);
				} 

			} 
			
			else {
				
				if(array_mutex[mutexid].propietario == p_proc_actual->id) {				
					return -1;
				}
				
				else {
					
					int nivel_int = fijar_nivel_int(NIVEL_3);

					
					p_proc_actual->estado=BLOQUEADO;
					
					eliminar_primero(&lista_listos);
					insertar_ultimo(&(array_mutex[mutexid].lista_proc_esperando_lock), p_proc_actual);
					
					
					BCP* p_proc_bloq = p_proc_actual; 
					p_proc_actual=planificador();
					cambio_contexto(&(p_proc_bloq->contexto_regs), &(p_proc_actual->contexto_regs));

					
					fijar_nivel_int(nivel_int);
				} 
			}
		} 
		
		else {
			array_mutex[mutexid].locked++;
			proceso_esperando = 0;
		} 
	} 

	array_mutex[mutexid].propietario = p_proc_actual->id;
	printk("Lock realizado sobre mutex\n");
	return 0;
}


int unlock (unsigned int mutexid) {

	int desc_proc=(unsigned int)leer_registro(1); 
	mutexid = p_proc_actual->descriptores[desc_proc];

	
	if(array_mutex[mutexid].abierto == 0) {
		return -1;
	}

	
	if(array_mutex[mutexid].locked > 0) {
		
		if((array_mutex[mutexid].tipo) == RECURSIVO) {
			if(array_mutex[mutexid].propietario == p_proc_actual->id) {

				array_mutex[mutexid].locked--;

				if(array_mutex[mutexid].locked == 0) {
					if(((array_mutex[mutexid].lista_proc_esperando_lock).primero) != NULL) {

						int nivel_int = fijar_nivel_int(NIVEL_3);
						BCP* proc_esperando = (array_mutex[mutexid].lista_proc_esperando_lock).primero;
						proc_esperando->estado = LISTO;
						eliminar_primero(&(array_mutex[mutexid].lista_proc_esperando_lock)); 
						insertar_ultimo(&lista_listos, proc_esperando);
						fijar_nivel_int(nivel_int);
					} 

					array_mutex[mutexid].propietario = -1;
				} 
			} 
			else {
				return -1;
			} 
		} 
		else {
			
			if(array_mutex[mutexid].propietario == p_proc_actual->id) {
	
				array_mutex[mutexid].locked--;
				array_mutex[mutexid].propietario = -1;

				if(((array_mutex[mutexid].lista_proc_esperando_lock).primero) != NULL) {
					int nivel_int = fijar_nivel_int(NIVEL_3);

					BCP* proc_esperando = (array_mutex[mutexid].lista_proc_esperando_lock).primero;
					proc_esperando->estado = LISTO;
					eliminar_primero(&(array_mutex[mutexid].lista_proc_esperando_lock)); 
					insertar_ultimo(&lista_listos, proc_esperando);

					fijar_nivel_int(nivel_int);
					
				} 
			} 
			else {
				return -1;
			}
		} 
	} 
	
	else {
		return -1;
	}

	printk("Unlock realizado correctamente\n");
	return 0;
}


int cerrar_mutex (unsigned int mutexid) {

	unsigned int registro=(unsigned int)leer_registro(1);
	if(registro < 16) {
		mutexid = registro;
	}
	
	int desc_mut = p_proc_actual->descriptores[mutexid];

	if(array_mutex[desc_mut].nombre == NULL) {
		return -1;
	}

	
	p_proc_actual->descriptores[mutexid] = -1;
	p_proc_actual->n_descriptores--;

	
	if(array_mutex[desc_mut].propietario == p_proc_actual->id) {
		
		array_mutex[desc_mut].locked = 0;

		while((array_mutex[desc_mut].lista_proc_esperando_lock).primero != NULL) {

			int nivel_int = fijar_nivel_int(NIVEL_3);

			BCP* proc_esperando = (array_mutex[desc_mut].lista_proc_esperando_lock).primero;
			proc_esperando->estado = LISTO;
			eliminar_primero(&(array_mutex[desc_mut].lista_proc_esperando_lock)); 
			insertar_ultimo(&lista_listos, proc_esperando);

			fijar_nivel_int(nivel_int);

		}
	
	}
	array_mutex[desc_mut].abierto--;

	
	if(array_mutex[desc_mut].abierto <= 0) {
		mutex_creados--;
		while(lista_bloq_mutex.primero != NULL) {
			int nivel_int = fijar_nivel_int(NIVEL_3);

			BCP* proc_esperando = lista_bloq_mutex.primero;
			proc_esperando->estado = LISTO;
			eliminar_primero(&lista_bloq_mutex); 
			insertar_ultimo(&lista_listos, proc_esperando);

			fijar_nivel_int(nivel_int);
			printk("Se ha desbloqueado el proceso\n");
		}
	}



	return 0;
}
/*
 *
 * Rutina de inicializaci�n invocada en arranque
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
