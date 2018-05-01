//Gonçalo Filipe Lucas Menino Rodrigues Pinto nº2014202176
//João Filipe Mendes Castilho nº2014202807

#include "main.h"


int id;
Stats* myStats;
int number_of_chars;
char str[256];
int fd,MQ;
int num_pac = 1;
int fdsrc;
char *src;
struct stat statbuf;
Q_TYPE* queue;
pthread_mutexattr_t attrmutex;

void initializeStruct(){
	pthread_mutex_init(&myStats->mutex_shared_memory, &attrmutex);
	pthread_mutex_init(&myStats->mutex_queue, &attrmutex);
	pthread_mutex_init(&myStats->mutex_message_queue, &attrmutex);
	pthread_mutex_init(&myStats->mutex_log_file, &attrmutex);
	myStats->num_processes = 0;
	myStats->triage = -1;
	myStats->doctors = -1;
	myStats->shift_length = -1;
	myStats->mq_max = -1;
	myStats->total_waiting = 0;
	myStats->triage_patients = 0;
	myStats->attended_patients = 0;
	myStats->triage_waiting = 0;
	myStats->service_waiting = 0;
	myStats->terminate = 0;
	myStats->alive_process = false;
	memset(myStats->aliveprocesses, 0 , 40*sizeof(int));
	memset(myStats->threadsalive, 0 , 40*sizeof(int));
	memset (myStats->processes, 0, 20*sizeof (pid_t) );
	memset (myStats->threads, 0, 40*sizeof (pthread_t) );
	memset (myStats->thread_ids, 0, 40*sizeof (int) );
}
int alivethread(int idthread){
	int i;
	pthread_mutex_lock(&myStats->mutex_shared_memory);
	for(i=0;i<=myStats->triage;i++){
		if(idthread == (int) myStats->threads[i]){
			pthread_mutex_unlock(&myStats->mutex_shared_memory);
			return myStats->threadsalive[i];
		}

	}

	pthread_mutex_unlock(&myStats->mutex_shared_memory);
	return 0;

}
int readconfigs(){
	char buff[255];
	char * token;
	int success = 1;	
	FILE *fp;
	fp = fopen("config.txt","r");
	if(fp==NULL){
		printf("The file doesn't exist. Please create a config file\n");
		return 0;	
	}
	while(fgets(buff,255,fp)!=NULL){
		token = strtok(buff,"=");
		if(strcmp(token,"TRIAGE")==0){
			token = strtok(NULL,"=");
			myStats->triage = atoi(token);
		}else if(strcmp(token,"DOCTORS")==0){
			token = strtok(NULL,"=");
			myStats->doctors = atoi(token);
		}else if(strcmp(token,"SHIFT_LENGTH")==0){
			token = strtok(NULL,"=");
			myStats->shift_length = atoi(token);
		}else if(strcmp(token,"MQ_MAX")==0){
			token = strtok(NULL,"=");
			myStats->mq_max = atoi(token);
		}

	}
	if(myStats->triage<=0){
		printf("Expected triage parameter!\n");
		success = 0;
	}if(myStats->doctors<=0){
		printf("Expected doctors parameter!\n");
		success = 0;
	}if(myStats->shift_length<=0){
		printf("Expected shift_length parameter!\n");
		success = 0;
	}if(myStats->mq_max<=0){
		printf("Expected mq_max parameter!\n");
		success = 0;
	}
	return success;

}
void print_stats(){
	pthread_mutex_lock(&myStats->mutex_shared_memory);
	printf("Numero total de pacientes triados: %d\n",myStats->triage_patients);
	printf("Numero total de pacientes atendidos: %d\n",myStats->attended_patients);
	printf("Tempo médio de espera antes do início da triagem: %.8f\n",(myStats->triage_waiting)/(myStats->triage_patients));
	printf("Tempo médio de espera entre o fim da triagem e o início do atendimento: %.8f\n",(myStats->service_waiting)/(myStats->attended_patients));
	printf("Média do tempo total que cada utilizador gastou desde que chegou ao sistema até sair: %.8f\n",(myStats->total_waiting)/(myStats->attended_patients));
	pthread_mutex_unlock(&myStats->mutex_shared_memory);

}	
void print_doctor_stats(){
    printf("Numero total de doutores criados:%d\n",myStats->attended_patients);
}
 
void doctorWork(int i){
	pthread_mutex_lock(&myStats->mutex_shared_memory);
	myStats->processes[i] = getpid();
	myStats->num_processes++;
	myStats->aliveprocesses[i] = 1;
	pthread_mutex_unlock(&myStats->mutex_shared_memory);
	Message m;
	time_t start;
	time_t temp;
	int num_messages;
	double seconds,all_time,sleep_time;
	time_t end;
	char aux[256];
	char hora[26];
	struct msqid_ds buf;
	struct tm aux_temp;
	pthread_mutex_lock(&myStats->mutex_message_queue);
	msgctl(MQ,IPC_STAT,&buf);
	pthread_mutex_unlock(&myStats->mutex_message_queue);

	num_messages = buf.msg_qnum;
	temp = time(0);
	aux_temp = (*localtime(&temp));
	strftime(hora, 30, "%Y-%m-%d %H:%M:%S", &aux_temp);
	printf("The turn of %ld is beggining!\n",(long)getpid());
	sprintf(aux,"Inicio do turno do doutor %ld em %s\n",(long)getpid(),hora);
	pthread_mutex_lock(&myStats->mutex_log_file);
	write(fdsrc,aux,strlen(aux));
	pthread_mutex_unlock(&myStats->mutex_log_file);
	pthread_mutex_lock(&myStats->mutex_shared_memory);
	time_t now = time(0),finalTurn = time(0) + myStats->shift_length;
	pthread_mutex_unlock(&myStats->mutex_shared_memory);
	while(now<finalTurn){
		pthread_mutex_lock(&myStats->mutex_shared_memory);
		if(!myStats->aliveprocesses[i]){
			pthread_mutex_unlock(&myStats->mutex_shared_memory);
			break;
		}
		pthread_mutex_unlock(&myStats->mutex_shared_memory);
		pthread_mutex_lock(&myStats->mutex_message_queue);		
		msgctl(MQ,IPC_STAT,&buf);
		pthread_mutex_unlock(&myStats->mutex_message_queue);	
		num_messages = buf.msg_qnum;
		if(num_messages>0){
			pthread_mutex_lock(&myStats->mutex_message_queue);
			msgrcv(MQ,&m,sizeof(m)-sizeof(long),-3,IPC_NOWAIT);
			pthread_mutex_unlock(&myStats->mutex_message_queue);
			start = time(NULL);
			seconds = difftime(start,mktime(&m.pac.saida_triagem));

			pthread_mutex_lock(&myStats->mutex_shared_memory);
			myStats->service_waiting+=seconds;
			pthread_mutex_unlock(&myStats->mutex_shared_memory);
			
			sleep_time = (m.pac.atendimento)*1000;
			aux_temp = (*localtime(&start));
			strftime(hora, 26, "%Y-%m-%d %H:%M:%S", &aux_temp);
			sprintf(aux,"Inicio do atendimento do paciente %s pelo doutor %ld em %s\n",m.pac.nome,(long)getpid(),hora);
			pthread_mutex_lock(&myStats->mutex_log_file);
			write(fdsrc,aux,strlen(aux));
			pthread_mutex_unlock(&myStats->mutex_log_file);

			usleep(sleep_time);
		
			end = time(NULL);
			aux_temp = (*localtime(&end));
			strftime(hora, 26, "%Y-%m-%d %H:%M:%S", &aux_temp);
			sprintf(aux,"Fim do atendimento do paciente %s pelo doutor %ld em %s\n",m.pac.nome,(long)getpid(),hora);
			pthread_mutex_lock(&myStats->mutex_log_file);
			write(fdsrc,aux,strlen(aux));
			pthread_mutex_unlock(&myStats->mutex_log_file);

			all_time = difftime(end,mktime(&m.pac.chegada));
			pthread_mutex_lock(&myStats->mutex_shared_memory);
			myStats->attended_patients++;
			myStats->total_waiting+=all_time;
			pthread_mutex_unlock(&myStats->mutex_shared_memory);

		}
		now = time(0);
	}
	temp = time(NULL);
	aux_temp = (*localtime(&temp));
	strftime(hora, 26, "%Y-%m-%d %H:%M:%S", &aux_temp);
	sprintf(aux,"Fim do turno do doutor %ld em %s\n",(long)getpid(),hora);
	pthread_mutex_lock(&myStats->mutex_log_file);
	write(fdsrc,aux,strlen(aux));
	pthread_mutex_unlock(&myStats->mutex_log_file);
	pthread_mutex_lock(&myStats->mutex_shared_memory);
	myStats->processes[i] = -1;
	myStats->num_processes--;
	myStats->aliveprocesses[i] = 0;
	pthread_mutex_unlock(&myStats->mutex_shared_memory);
	printf("The turn of %ld is finishing. This proccess will close!\n",(long)getpid());
}

void create_shmem_vars(){
	if((id = shmget(6005,sizeof(&myStats),IPC_CREAT | 0777))<0){
		printf("SHMGET ERRdOR. Program will close.\n");
		exit(1);
	}
	if((myStats = (Stats*) shmat(id,NULL,0))==(Stats*)-1){
		printf("SHMAT ERROR. Program will close.\n");
		exit(1);
	}

}
int findNumprocesses(){
	int i;
	pthread_mutex_lock(&myStats->mutex_shared_memory);
	for(i=0;i<myStats->doctors+1;i++){
		if(myStats->processes[i]==-1){
			pthread_mutex_unlock(&myStats->mutex_shared_memory);
			return i;
		}
	}
	pthread_mutex_unlock(&myStats->mutex_shared_memory);
	return -1;
}
void create_doctor_processes(){
	int i;
	pid_t doctor;

	pthread_mutex_lock(&myStats->mutex_shared_memory);
	int doctors = myStats->doctors;
	pthread_mutex_unlock(&myStats->mutex_shared_memory);	
	for(i=0;i<doctors;i++){
		doctor = fork();
		if(doctor == 0){
       			signal (SIGINT, SIG_IGN);
			doctorWork(i);
			exit(0);
		}
		else if(doctor < 0)
			exit(1);
	}

	int status =0;
	int numproc,num_messages,mqmax;
	pid_t new_process;
	bool temporary = false;
	bool firsttime = true;
	int terminate;
	struct msqid_ds buf;
	while (wait(&status) > 0){
		pthread_mutex_lock(&myStats->mutex_shared_memory);
		if(!myStats->terminate){
			mqmax = myStats->mq_max;
			pthread_mutex_unlock(&myStats->mutex_shared_memory);
			pthread_mutex_lock(&myStats->mutex_message_queue);	
			msgctl(MQ,IPC_STAT,&buf);
			pthread_mutex_unlock(&myStats->mutex_message_queue);	
			num_messages = buf.msg_qnum;
			if(num_messages > mqmax && !temporary){
				temporary = true;
			}
			else if(temporary && num_messages < (0.8*mqmax)){
				temporary = false;
				firsttime = true;
				continue;			
			}
			if(temporary && !myStats->alive_process){
				myStats->alive_process = true;
				numproc = findNumprocesses();
				while(numproc == -1)
					numproc = findNumprocesses();			
				new_process = fork();
				if(new_process == 0){
       					signal (SIGINT, SIG_IGN);
					printf("O Processo extra %ld vai ser criado\n",(long)getpid());
					doctorWork(numproc);
					printf("O Processo extra %ld terminou\n",(long)getpid());
					myStats->alive_process = false;
					exit(0);
				}
				else if(new_process < 0)
					exit(1);
				if(firsttime)
					firsttime = false;
				else
					continue;	
			}
			numproc = findNumprocesses();
			doctor = fork();
			if(doctor == 0){
      				signal (SIGINT, SIG_IGN);
				doctorWork(numproc);
				exit(0);
			}
			else if(doctor < 0)
				exit(1);
		}
		else{
			printf("O programa está a acabar...\n");
			pthread_mutex_unlock(&myStats->mutex_shared_memory);
		}
	}
}

int empty_queue (){
	return (queue->front == NULL ? 1 : 0);
}

Paciente dequeue (){
	NODE_TYPE *temp_ptr;
	Paciente pac;
	pthread_mutex_lock(&myStats->mutex_queue);
	if (empty_queue (queue) == 0) {
		temp_ptr = queue->front;
		pac = temp_ptr->pac;
		queue->front = queue->front->next;
		if (empty_queue (queue) == 1)
			queue->rear = NULL;
		free (temp_ptr);
		pthread_mutex_unlock(&myStats->mutex_queue);
		return pac;
	}
	pthread_mutex_unlock(&myStats->mutex_queue);
	return pac;
}

void destroy_queue (){
	NODE_TYPE *temp_ptr;
	pthread_mutex_lock(&myStats->mutex_queue);
	while (empty_queue (queue) == 0) {
		temp_ptr = queue->front;
		queue->front = queue->front->next;
		free (temp_ptr);
	}
	queue->rear = NULL;
	pthread_mutex_unlock(&myStats->mutex_queue);
}
void killallprocesses(){
	int i,doctors,process;
	pthread_mutex_lock(&myStats->mutex_shared_memory);
	doctors = myStats->doctors;
	pthread_mutex_unlock(&myStats->mutex_shared_memory);
	for(i=0;i<doctors;i++){
		pthread_mutex_lock(&myStats->mutex_shared_memory);
		process = myStats->processes[i];
		pthread_mutex_unlock(&myStats->mutex_shared_memory);
		if(process!=-1){
			pthread_mutex_lock(&myStats->mutex_shared_memory);
			kill(myStats->processes[i],SIGKILL);
			printf("The process %d was forced to die!\n",myStats->processes[i]);
			pthread_mutex_unlock(&myStats->mutex_shared_memory);
		}
	}
}
void sig_header(int sign){
	int i,doctors,triage,process;
	if(sign == SIGUSR1){
		print_stats();
	}
	if (sign == SIGINT){
		pthread_mutex_lock(&myStats->mutex_shared_memory);
		myStats->terminate = 1;
		doctors = myStats->doctors;
		triage = myStats->triage;

		for(i=0;i<doctors;i++){
			if(myStats->processes[i]!=-1){
				myStats->aliveprocesses[i] = 0;
			}

		}
		pthread_mutex_unlock(&myStats->mutex_shared_memory);
		time_t now = time(0);
		pthread_mutex_lock(&myStats->mutex_shared_memory);
		time_t finalturn = now + myStats->shift_length;
		int num_processes = myStats->num_processes;
		pthread_mutex_unlock(&myStats->mutex_shared_memory);
		while(num_processes>0){
			if(now>finalturn){
				killallprocesses();
				break;
			}
			pthread_mutex_lock(&myStats->mutex_shared_memory);
			num_processes = myStats->num_processes;
			pthread_mutex_unlock(&myStats->mutex_shared_memory);
			now = time(0);	
		}
		pthread_t thread;
		for(i=0;i<=triage;i++){
			pthread_mutex_lock(&myStats->mutex_shared_memory);
			myStats->threadsalive[i] = 0;
			thread = myStats->threads[i];
			pthread_mutex_unlock(&myStats->mutex_shared_memory);
			pthread_join(thread,NULL);
        	}

		msgctl(MQ,IPC_RMID,NULL);
		destroy_queue();
		close(fd);
		pthread_mutex_destroy(&myStats->mutex_shared_memory);
		pthread_mutex_destroy(&myStats->mutex_log_file);
		pthread_mutex_destroy(&myStats->mutex_message_queue);
		pthread_mutex_destroy(&myStats->mutex_queue);
		pthread_mutexattr_destroy(&attrmutex);
		printf("Semaphores destroyed\n");
		shmdt(myStats);
		shmctl(id, IPC_RMID, NULL);
		printf("Shared memory destroyed\n");
		printf("The program will close!\n");

		exit(0);
	}

}
void* toDoThread(){

	Message m;
	double sleep_time;
	Paciente pac;
	double seconds;
	time_t now,end_triage;
	struct tm aux_temp;
	char aux[256];
	char hora[26];
	while(alivethread(pthread_self())){
		pthread_mutex_lock(&myStats->mutex_queue);
		if(!empty_queue(queue)){
			pthread_mutex_unlock(&myStats->mutex_queue);
			pac = dequeue();

			if(pac.valid==true){
				now = time(NULL);
				seconds = difftime(now,mktime(&pac.chegada));
				pthread_mutex_lock(&myStats->mutex_shared_memory);
				myStats->triage_waiting+=seconds;
				pthread_mutex_unlock(&myStats->mutex_shared_memory);

				m.msgtype = pac.prioridade;
				sleep_time = (pac.triagem)*1000;

				aux_temp = (*localtime(&now));
				strftime(hora, 26, "%Y-%m-%d %H:%M:%S", &aux_temp);
				sprintf(aux,"Paciente %s retirado da fila de espera em %s\n",pac.nome,hora);
				pthread_mutex_lock(&myStats->mutex_log_file);								
				write(fdsrc,aux,strlen(aux));
				pthread_mutex_unlock(&myStats->mutex_log_file);
				usleep(sleep_time);

				pthread_mutex_lock(&myStats->mutex_shared_memory);
				myStats->triage_patients++;
				pthread_mutex_unlock(&myStats->mutex_shared_memory);

				
				end_triage = time(NULL);
				pac.saida_triagem = (*localtime(&end_triage));
				m.pac = pac;
				
				strftime(hora, 26, "%Y-%m-%d %H:%M:%S", &pac.saida_triagem);
				sprintf(aux,"Paciente %s inserido na fila de mensagens em %s\n",pac.nome,hora);
				pthread_mutex_lock(&myStats->mutex_log_file);				
				write(fdsrc,aux,strlen(aux));
				pthread_mutex_unlock(&myStats->mutex_log_file);

				pthread_mutex_lock(&myStats->mutex_message_queue);
				msgsnd(MQ,&m,sizeof(m)-sizeof(long),IPC_NOWAIT);
				pthread_mutex_unlock(&myStats->mutex_message_queue);
			}
		}
		else{
			pthread_mutex_unlock(&myStats->mutex_queue);	
		}
	}
	printf("A thread %ld vai morrer!\n",(long)pthread_self());
	pthread_exit(NULL);
	return NULL;

}

void enqueue (Paciente pac){
	NODE_TYPE *temp_ptr;
	temp_ptr = (NODE_TYPE*) malloc (sizeof (NODE_TYPE));
	if (temp_ptr != NULL) {
		temp_ptr->pac = pac;
		temp_ptr->next = NULL;
		if (empty_queue (queue) == 1)
			queue->front = temp_ptr;
		else queue->rear->next = temp_ptr;
			queue->rear = temp_ptr; 
	}
	else{
		fprintf(stderr,"Failed to allocate memory.\n");
		exit(0);
	}
}

void* escuta_pipe(){
	char * token;
	char* token2;
	char* person;
	int i;
	int k;
	char* str1, *str2;
	char* command;
	char aux[256];
	time_t now;
	int numtriage;
	Paciente pac;
	pac.valid= true;
	while (alivethread(pthread_self())) {
		fd_set read_set;
		FD_ZERO(&read_set);
		FD_SET(fd,&read_set);
		char hora[26];
		if(select(fd+1,&read_set,NULL,NULL,NULL)>0){
			if(FD_ISSET(fd,&read_set)){
				if((number_of_chars=read(fd, str, sizeof(str)))>0){
					str[number_of_chars-1]='\0';
					str1 = str;
					while(token = strtok_r(str1,"\n",&str1)){
						str2 = token;
						token2 = strtok_r(str2,"=",&command);
						if((token2 = strtok_r(command,"=",&command)) != NULL){ //Se recebe um =, logo é um comando especial
							numtriage = atoi(token2);
							pthread_mutex_lock(&myStats->mutex_shared_memory);
							if(numtriage > myStats->triage){
								for(i=myStats->triage + 1;i < numtriage + 1;i++){
									printf("\n\nCreating the thread number %d\n\n\n",i);
									myStats->thread_ids[i] = i;
									pthread_create(&myStats->threads[i],NULL,toDoThread,&myStats->thread_ids[i]);	
								}
								myStats->triage = numtriage;
							}
							else if(numtriage < myStats->triage){
								for(i=myStats->triage;i>numtriage;i--){
									printf("\n\nEliminating the thread number %ld\n\n\n",(long) myStats->threads[i]);
                							myStats->threadsalive[i] = 0;
								}
								myStats->triage = numtriage;
							}
							pthread_mutex_unlock(&myStats-> mutex_shared_memory );																
						}
						token2 = strtok_r(str2," ",&str2);
						person = token2;
						k=1;
						while(token2 = strtok_r(str2," ",&str2)){
							switch(k){
								case 1:
									pac.triagem = atoi(token2);
									break;
								case 2:
									pac.atendimento = atoi(token2);
									break;
								case 3:
									pac.prioridade = atoi(token2);
									break;
							}
							k++;
						}
						if(atoi(person)==0){ //se o primeiro elemento nao for um numero, ou seja, o pedido é individual
							strcpy(pac.nome,person);
							pac.num_chegada = num_pac++;
							now = time(NULL);
							pac.chegada = (*localtime(&now));
		    					strftime(hora, 26, "%Y-%m-%d %H:%M:%S", &pac.chegada);
							pthread_mutex_lock(&myStats->mutex_queue);
							enqueue(pac);
							pthread_mutex_unlock(&myStats->mutex_queue);
							sprintf(aux,"Paciente %s colocado na fila de espera em %s\n",pac.nome,hora);
							pthread_mutex_lock(&myStats->mutex_log_file);				
							write(fdsrc,aux,strlen(aux));
							pthread_mutex_unlock(&myStats->mutex_log_file);
						}
						else{
							strcpy(pac.nome,"Paciente Grupo");
							for(i=0;i<atoi(person);i++){
								pac.num_chegada = num_pac++;
								now = time(NULL);
								pac.chegada = (*localtime(&now));
			    					strftime(hora, 26, "%Y-%m-%d %H:%M:%S", &pac.chegada);
								pthread_mutex_lock(&myStats->mutex_queue);
								enqueue(pac);
								pthread_mutex_unlock(&myStats->mutex_queue);
								sprintf(aux,"Paciente %s colocado na fila de espera em %s\n",pac.nome,hora);
								pthread_mutex_lock(&myStats->mutex_log_file);				
								write(fdsrc,aux,strlen(aux));
								pthread_mutex_unlock(&myStats->mutex_log_file);
							}
						}


					}
				}
			}
		}

	}
	return NULL;
}


void create_threads(){
	int i;
	pthread_mutex_lock(&myStats->mutex_shared_memory);
	for(i=1;i<myStats->triage+1;i++){
		
		myStats->thread_ids[i] = i;
		pthread_create(&myStats->threads[i],NULL,toDoThread,&myStats->thread_ids[i]);
		myStats->threadsalive[i] = 1;		
	}
	myStats->thread_ids[0] = 0;
	pthread_create(&myStats->threads[0],NULL,escuta_pipe,&myStats->thread_ids[0]);
	myStats->threadsalive[0] = 1;
	pthread_mutex_unlock(&myStats->mutex_shared_memory);
}

void create_queue (){
	queue = (Q_TYPE *)malloc(sizeof(queue));
	if(queue==NULL){
		fprintf(stderr,"Failed to allocate memory.\n");
		exit(0);
	}
	queue->front = NULL;
	queue->rear = NULL;
}



int main(){
	pthread_mutexattr_init(&attrmutex);
	pthread_mutexattr_setpshared(&attrmutex, PTHREAD_PROCESS_SHARED);
	printf("Opening log file...\n");	
	if ((fdsrc = open ("ficheiro.log", O_RDWR | O_CREAT | O_TRUNC , FILE_MODE)) < 0){
      		fprintf(stderr,"dst open error: %s\n",strerror(errno));
      		exit(1);
   	}
	lseek(fdsrc,499,SEEK_SET);
	write(fdsrc,"",1);
	if((src = mmap (0, 500, PROT_READ | PROT_WRITE, MAP_SHARED, fdsrc, 0)) == (caddr_t) -1){
      		fprintf(stderr,"dst mmap error: %s\n",strerror(errno));
      		exit(1);
  	}
	lseek(fdsrc,0,SEEK_SET);
	printf("Log file opened\n");	
	printf("Creating named pipe...\n");
	if ((mkfifo(PIPE_NAME, O_CREAT|O_EXCL|0600)<0) && (errno!= EEXIST)){
		perror("Cannot create pipe");
		exit(0);
	}
	printf("Named pipe created!\n");
	printf("Opening named pipe for reading...\n");
	if ((fd = open(PIPE_NAME, O_RDONLY | O_NONBLOCK)) < 0) {
		perror("Cannot open pipe for reading");
		exit(0);
	}
	printf("Named pipe opened for reading!\n");
	if((MQ = msgget(IPC_PRIVATE,IPC_CREAT|0700)) < 0){
		perror("Cannot create message queue");
		exit(0);
	}
	create_queue (queue);
	printf("Creating shared memory...\n");
	create_shmem_vars();
	printf("Shared Memory created!\n");
	initializeStruct();
	printf("The program is open!\n");
	printf("Trying to read from config file...\n");
	if (signal(SIGINT, sig_header) == SIG_ERR)
	  printf("\ncan't catch SIGINT\n");
	if(signal(SIGUSR1,sig_header) == SIG_ERR)
	  printf("\ncan't catch SIGUSR1\n");
	if(readconfigs())
		printf("Read successfully!\n");
	else{
		printf("That was a problem trying to read the config file! The program will close.\n");
		return -1;	
	}
	printf("Creating threads...\n");
	create_threads();
	printf("All threads created!\n");
	printf("Creating doctor processes...\n");
	create_doctor_processes();
	return 0;
}
