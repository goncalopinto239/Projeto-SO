//Gonçalo Filipe Lucas Menino Rodrigues Pinto nº2014202176
//João Filipe Mendes Castilho nº2014202807

#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <ctype.h>
#include <sys/mman.h>
#include <semaphore.h>

#define PIPE_NAME "input_pipe"
#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

typedef struct stats{	
	pthread_mutex_t mutex_shared_memory;
	pthread_mutex_t mutex_queue;
	pthread_mutex_t mutex_message_queue;
	pthread_mutex_t mutex_log_file;
	int num_processes;
	pid_t processes[40];
	bool alive_process;
	int terminate;
	int aliveprocesses[40];
	pthread_t threads[40];
	int thread_ids[40];
	int threadsalive[40];
	int triage;
	int doctors;
	int shift_length;
	int mq_max;
	int triage_patients; //pacientes triados
	int attended_patients; //pacientes atendidos
	double triage_waiting; //tempo espera na triagem em seg
	double service_waiting; //tempo de espera no servico em seg
	double total_waiting; //tempo de espera total em seg

}Stats;	

typedef struct data{
	int dia;
    	int mes;
    	int ano;
    	int horas;
    	int minutos;
}Data;

typedef struct paciente{
	
	bool valid;
    	char nome[50];
    	int prioridade;
    	int num_chegada;
	int triagem;
	int atendimento;
	struct tm chegada;
	struct tm saida_triagem;
}Paciente;

typedef struct node_type{
	Paciente pac;
	struct node_type *next;
}NODE_TYPE;

typedef struct q_type{
	NODE_TYPE *rear;
	NODE_TYPE *front;
}Q_TYPE;

typedef struct message{
	long msgtype;
	Paciente pac;
}Message;

#endif // MAIN_H_INCLUDED
