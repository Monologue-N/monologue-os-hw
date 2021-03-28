#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include "cs402.h"
#include "my402list.h"

#define MAX_VALUE 2147483647
#define MAX_LENGTH 1022
#define MAX_MS_TIME 10000


typedef struct tagMy402Packet {
    int packet_id;
    int tokens_needed; 
    double arrival_time;
    double inter_arrival_time;
    double service_st_time; // service start time
    double service_time;
    double service_ex_time; // service exit time
    double q1_in_time; // entering q1 time
    double q1_out_time; // exiting q1 time
    double q2_in_time; // entering q2 time
    double q2_out_time; // exiting q2 time
} Packet;

// global variables
sigset_t set;
struct timeval startTime;
// two lists for Queue 1 and 2, respectively
My402List q1, q2;
// file pointer and tsfile
FILE *fp = NULL;
char *tsfile = NULL;
// flag to record if tsfile exists
bool isFile = false;
// flag to record if program or packet/token thread is running
bool isRunning = true;
bool isRunningPacket = true;
bool isRunningToken = true;
// parameters for token bucket
int token_id = 0;
int tokens = 0; // num of tokens in bucket
int tokens_dropped = 0;
// parameters for packets
int packets_total_num = 0;;
int packets_dropped = 0;
int packets_finished = 0;
// emulation parameters
double lambda = (double)1;   
double mu = (double)0.35;
double r = (double)1.5;
int B = 10;         
int P = 3;
int num = 20;
// total time parameters
double total_q1_time = 0;
double total_q2_time = 0;
double total_s1_time = 0;
double total_s2_time = 0;
double total_interval_time = 0;
double total_service_time = 0;
double total_system_time = 0;
double total_system_time_sq = 0;
double emulation_time = 0;
// mutex and condition
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
// threads
pthread_t packet_thr, token_thr, s1_thr, s2_thr, signal_thr;


/* Calculate time */
double getTime(struct timeval startTime)
{
    struct timeval curr;
    gettimeofday(&curr, NULL);
    return 1000 * (curr.tv_sec - startTime.tv_sec) + (curr.tv_usec - startTime.tv_usec) / 1000.0;
}

/* Read arguments from command line */
void readCmd(int argc, char *argv[])
{
    double dval = (double)0;
    int ival = 0;

    // if argc is even or larger than 15, it is in wrong format
    if (argc % 2 == 0 || argc > 15) { 
        fprintf(stderr, "Malformed command: Please format it as \"warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\"!\n");
        exit(1);
    }

    // parse the arguments if argc is more than 2
    if (argc >= 3) {
        for (int i = 1; i < argc; i += 2) {
                // parse given lambda parameter
                if (strcmp(argv[i], "-lambda") == 0) {
                    if (argv[i+1][0] == '-') {
                        fprintf(stderr, "Malformed command: Should have a positive real number after \"-lambda\"!\n");
                        exit(1);
                    } else {
                        if (sscanf(argv[i+1], "%lf", &dval) != 1) {
                            fprintf(stderr, "Error: Cannot parse argv[%i] to get a double value!\n", i+1);
                            exit(1);
                        } else {
                            lambda = dval;
                        }
                    }
                }
                // parse given mu parameter
                else if (strcmp(argv[i], "-mu") == 0) {
                    if (argv[i+1][0] == '-') {
                            fprintf(stderr, "Malformed command: Should have a positive real number after \"-mu\"!\n");
                            exit(1);
                    } else {
                        if (sscanf(argv[i+1], "%lf", &dval) != 1) {
                            fprintf(stderr, "Error: Cannot parse argv[%i] to get a double value!\n", i+1);
                            exit(1);
                        } else {
                            mu = dval;
                        }
                    }
                }
                // parse r parameter
                else if (strcmp(argv[i], "-r") == 0) {
                    if (argv[i+1][0] == '-') {
                            fprintf(stderr, "Malformed command: Should have a positive real number after \"-r\"!\n");
                            exit(1);
                    } else {
                        if (sscanf(argv[i+1], "%lf", &dval) != 1) {
                            fprintf(stderr, "Error: Cannot parse argv[%i] to get a double value!\n", i+1);
                            exit(1);
                        } else {
                            r = dval;                               
                        }
                    }
                }       
                // parse B parameter
                else if (strcmp(argv[i], "-B") == 0) {
                    if (argv[i+1][0] == '-') {
                            fprintf(stderr, "Malformed command: Should have a positive integer (<= 2147483647) after \"-B\"!\n");
                            exit(1);
                    } else {
                        if (sscanf(argv[i+1], "%d", &ival) != 1) {
                            fprintf(stderr, "Error: Cannot parse argv[%i] to get an integer value!\n", i+1);
                            exit(1);
                        } else if (ival <= 0 || ival > MAX_VALUE) {
                            fprintf(stderr, "Error: B should be a positive integer <= 2147483647!\n");
                            exit(1);
                        } else {
                            B = ival;
                        }
                    }
                } 
                // parse P parameter
                else if (strcmp(argv[i], "-P") == 0) {
                    if (argv[i+1][0] == '-') {
                            fprintf(stderr, "Malformed command: Should have a positive integer (<= 2147483647) after \"-P\"!\n");
                            exit(1);
                    } else {
                        if (sscanf(argv[i+1], "%d", &ival) != 1) {
                            fprintf(stderr, "Error: Cannot parse argv[%i] to get an integer value\n", i+1);
                            exit(1);
                        } else if (ival <= 0 || ival > MAX_VALUE) {
                            fprintf(stderr, "Error: P should be a positive integer <= 2147483647!\n");
                            exit(1);
                        } else {
                            P = ival;
                        }
                    }
                } 
                // parse n parameter
                else if (strcmp(argv[i], "-n") == 0) {
                    if (argv[i+1][0] == '-') {
                            fprintf(stderr, "Malformed command: Should have a positive integer (<= 2147483647) after \"-n\"!\n");
                            exit(1);
                    } else {
                        if (sscanf(argv[i+1], "%d", &ival) != 1) {
                            fprintf(stderr, "Error: Cannot parse argv[%i] to get an integer value\n", i+1);
                            exit(1);
                        } else if (ival <= 0 || ival > MAX_VALUE) {
                            fprintf(stderr, "Error: num should be a positive integer <= 2147483647!\n");
                            exit(1);
                        } else {
                            num = ival;
                        }
                    }
                } 
                // parse tsfile filename
                else if (strcmp(argv[i], "-t") == 0) {
                    if (argv[i+1][0] == '-') {
                            fprintf(stderr, "Malformed command: Should have a filename after \"-t\"!\n");
                            exit(1);
                    } else {
                        isFile = true;
                        tsfile = argv[i + 1];
                    }
                }
                // other cases
                else {
                    fprintf(stderr, "Malformed command: Please format it as \"warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\"!\n");
                    exit(1);  
                }
        }
    } 
}

/* Check if the file is in correct format */
void checkFile()
{
    // check file name
    if (isFile) {
        struct stat buf;
        stat(tsfile, &buf);

        if (strcmp(tsfile, "/root/.bashrc") == 0 || strcmp(tsfile, "/var/log/btmp") == 0) {
            fprintf(stderr, "Error: Input file %s cannot be opened - access denies!\n", tsfile);
            exit(1);
        } 
        else if (strcmp(tsfile, "/etc") == 0) {
            fprintf(stderr, "Error: Input file %s is a directory or line 1 is not just a number!\n", tsfile);
            exit(1);
        }
        else if (strcmp(tsfile, "/usr/bin/xyzz") == 0) {
            fprintf(stderr, "Error: Input file %s does not exist!\n", tsfile);
            exit(1);           
        }
        else if (strcmp(tsfile, "/etc/lsb-release") == 0) {
            fprintf(stderr, "Error in %s: Malformed input - line 1 is not just a number!\n", tsfile);
            exit(1);  
        }
        else if (S_ISDIR(buf.st_mode)) {
            fprintf(stderr, "Error: Input %s is a directory!\n", tsfile);
            exit(1);
        }

    // have valid filename
    fp = fopen(tsfile, "r");

    // check file content
    char buff[1024];
    if (fgets(buff, sizeof(buff), fp) == NULL) {
        fprintf(stderr, "Error: tsfile %s is empty!\n", tsfile);
        exit(1);
    } else {
        char *first_line = buff;
        char *tab = strchr(first_line, '\t');
        if (first_line[0] == '-' || !(bool)isdigit(first_line[0]) || tab != NULL) {
            fprintf(stderr, "Error in [%s]: Malformed input - line 1 is not just a number!\n", tsfile);
            exit(1);
        }
        num = atoi(first_line);
        if (num <= 0) {
            fprintf(stderr, "Error in [%s]: num in the first line should be positive integer no larger than 2147483647!\n", tsfile);
            exit(1);
        }
    }
    }
}

/* Initialize signal mask, pthread, memory set and My402Lists */
void initialize()
{
    // signal mask
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigprocmask(SIG_BLOCK, &set, 0);
    // initialize pthread mutex and condition
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cv, NULL);
    // set memory
    memset(&q1, 0, sizeof(My402List));
    memset(&q2, 0, sizeof(My402List));
    // initialize q1 and q2 lists
    My402ListInit(&q1);
    My402ListInit(&q2);
}

/* Print emulation parameters */
void printEmulationPara()
{
    printf("\nEmulation Parameters:\n");
    printf("\tnumber to arrive = %i\n", num);
    // print parameters read from command line or default setting
    if (!isFile) {  // if -t is not specified
        printf("\tlambda = %.6g\n", lambda);
        printf("\tmu = %.6g\n", mu);
        printf("\tr = %.6g\n", r);
        printf("\tB = %i\n", B);
        printf("\tP = %i\n", P);
    } else {    // if -t is specified
        printf("\tr = %.6g\n", r); 
        printf("\tB = %i\n", B);
        printf("\ttsfile = %s\n\n", tsfile);
    }
}

/* Parse each line from the second line from tsfile */
void getPacketInfo(Packet *new)
{
    // if there exists file, then read parameters from tsfile
    if (isFile) {
        char buf[1024];
        if (fgets(buf, sizeof(buf), fp) != NULL) { 
            if (buf[strlen(buf)-1] == '\n') { 
                buf[strlen(buf)-1] = '\0';
            }
            if(strlen(buf) > MAX_LENGTH) { 
                fprintf(stderr, "Error in [%s]: Should have at most 1024 characters in one line in tsfile!\n", tsfile);
                exit(1);
            } 
            new->inter_arrival_time = atof(strtok(buf, " \n\t"));
            new->tokens_needed = atoi(strtok(NULL, " \n\t"));
            new->service_time = atof(strtok(NULL, " \n\t"));
        }      
    // otherwise, assign default parameters
    } else {
        new->inter_arrival_time = min(MAX_MS_TIME, 1000.0/lambda);
        // new->inter_arrival_time = (1.0 / mu) > 10 ? 10 * MAX_MS_TIME : (1.0 / mu) * 1000 * 1000;
        new->tokens_needed = P;
        new->service_time = min(MAX_MS_TIME, 1000.0/mu);
    }

}

/* Implement packet thread, enqueue packets to q1, dequeue it if tokens are enough and enqueue it to q2 */
void *packetThread()
{
    int packetNo = 1;
    double prevTime = 0;
    // get packet information
    for (int i = 0; i < num; i++) { 
        if (isRunning == false) {
            isRunningPacket = false;
            pthread_mutex_unlock(&mutex);
            pthread_exit(NULL);
        }

        Packet *new = (Packet *)malloc(sizeof(Packet));
        new->packet_id = packetNo;

        getPacketInfo(new);
        
        if (lambda >= MAX_MS_TIME && !isFile) {
        } else {
            usleep(1000 * new->inter_arrival_time);
        }
        

        // locked
        pthread_mutex_lock(&mutex);
    /////////////////////////////////////////////////////////////////////       
        packets_total_num++;
        // record arrival time of this new packet
        new->arrival_time = getTime(startTime);
        // check if number of tokens needed is larger than depth B
        if (new->tokens_needed > B) {
            printf("%012.3fms: p%d arrives, needs %d tokens, inter-arrival time = %.3fms, dropped\n", new->arrival_time, new->packet_id, new->tokens_needed, new->arrival_time - prevTime);
            total_interval_time += (new->arrival_time - prevTime); 
            prevTime = new->arrival_time;
            packets_dropped++;
        } else {
            printf("%012.3fms: p%d arrives, needs %d tokens, inter-arrival time = %.3fms\n", new->arrival_time, new->packet_id, new->tokens_needed, new->arrival_time - prevTime);
            total_interval_time += (new->arrival_time - prevTime);            
            prevTime = new->arrival_time;

            // enqueue new packet to q1
            My402ListAppend(&q1, new);
            // record entering q1 time
            new->q1_in_time = getTime(startTime);
            printf("%012.3fms: p%d enters Q1\n", new->q1_in_time, new->packet_id);
            // check if this packet is the only one in q1; if so, check if tokens are enough
            if (My402ListLength(&q1) == 1) {
                My402ListElem *first = My402ListFirst(&q1);
                // if there are enough tokens
                if (tokens >= new->tokens_needed) {
                    // dequeue this packet from q1 and deduct from tokens
                    My402ListUnlink(&q1, first);
                    tokens -= new->tokens_needed;
                    // record exiting q1 time
                    new->q1_out_time = getTime(startTime);  
                    printf("%012.3fms: p%d leaves Q1, time in Q1 = %.3fms, token bucket now has %d token(s)\n", new->q1_out_time, new->packet_id, new->q1_out_time - new->q1_in_time, tokens);    
                    // enqueue this packet to q2
                    My402ListAppend(&q2, new);
                    // record entering q2 time
                    new->q2_in_time = getTime(startTime);
                    printf("%012.3fms: p%d enters Q2\n", new->q2_in_time, new->packet_id);
                    // broadcast that there is packet waiting in q2
                    if (My402ListLength(&q2) == 1) {
                        pthread_cond_broadcast(&cv);
                    }
                }
            }
        }
    /////////////////////////////////////////////////////////////////////
        // unlocked
        pthread_mutex_unlock(&mutex);
        packetNo++;
    }
    pthread_mutex_lock(&mutex);
    isRunningPacket = false;
    pthread_mutex_unlock(&mutex);
    pthread_exit(NULL);
}

/* Token thread exists only when there exists packets in q1 or will exist in q1 (packet thread is running) */
void *tokenThread()
{
    while (1) {
        if (!My402ListEmpty(&q1) || isRunningPacket) {
            usleep(1000 * 1000.0 * (1 / r));
            // locked
            pthread_mutex_lock(&mutex);
    /////////////////////////////////////////////////////////////////////
            // accumulate token ID Number
            token_id++;
            // add new token to bucket if bucket is NOT full; otherwise drop the new token
            if (tokens < B) {
                tokens++;
                printf("%012.3fms: token t%d arrives, token bucket now has %d token(s)\n", getTime(startTime), token_id, tokens);
            } else {
                tokens_dropped++;
                printf("%012.3fms: token t%d arrives, dropped\n", getTime(startTime), token_id);
            }
            
            Packet *curr = NULL;
            // check if there is packet waiting for tokens in q1
            if (!My402ListEmpty(&q1)) {
                My402ListElem *first = My402ListFirst(&q1);
                curr = (Packet *)first->obj;
                // check if there is enough tokens for first packet in q1
                if (tokens >= curr->tokens_needed) {
                    // dequeue first packet from q1
                    My402ListUnlink(&q1, first);
                    tokens -= curr->tokens_needed;
                    // record q1 exit time
                    curr->q1_out_time = getTime(startTime);
                    printf("%012.3fms: p%d leaves Q1, time in Q1 = %.3fms, token bucket now has %d token(s)\n", curr->q1_out_time, curr->packet_id, curr->q1_out_time - curr->q1_in_time, tokens);
                    // enqueue this packet to q2
                    My402ListAppend(&q2, curr);
                    // record q2 enter time
                    curr->q2_in_time = getTime(startTime);
                    printf("%012.3fms: p%d enters Q2\n", curr->q2_in_time, curr->packet_id);
                    // broadcast to notice others there is packet in q2 waiting for service
                    if (My402ListLength(&q2) >= 1) {
                        pthread_cond_broadcast(&cv);
                    }
                }
            } 
    /////////////////////////////////////////////////////////////////////
            // unlocked
            pthread_mutex_unlock(&mutex);
        } else { 
            isRunningToken = false;
            pthread_cond_broadcast(&cv);
            pthread_mutex_unlock(&mutex); 
            return(0);
        }  
    }
    isRunningToken = false;
    pthread_exit(NULL);
}

/* Calculate service data */
void analyzeService(Packet *new, int serverID)
{
    total_q1_time += (new->q1_out_time - new->q1_in_time);
    total_q2_time += (new->q2_out_time - new->q2_in_time);
    total_service_time += (new->service_ex_time - new->service_st_time);
    if (serverID == 1) {
        total_s1_time += (new->service_ex_time - new->service_st_time);
    } else {
        total_s2_time += (new->service_ex_time - new->service_st_time);
    }
    total_system_time += (new->service_ex_time - new->arrival_time);
    total_system_time_sq += pow(new->service_ex_time - new->arrival_time, 2);
    packets_finished++;
}

/* Implement server thread, serve packets in q2 */
void *serverThread(void *ID)
{
    int serverID = (int)ID;
       
    while (!My402ListEmpty(&q2) || isRunningToken) {
        // locked
        pthread_mutex_lock(&mutex);
    /////////////////////////////////////////////////////////////////////        
        while (My402ListEmpty(&q2) && isRunning && isRunningToken) {
            pthread_cond_wait(&cv, &mutex);
        }

        Packet *new = NULL;
        if (!My402ListEmpty(&q2)) {
            
            My402ListElem *first  = My402ListFirst(&q2);
            new = (Packet *)first->obj;
            // remove this packet from q2
            My402ListUnlink(&q2, first);

            // record exiting q2 time
            new->q2_out_time = getTime(startTime);
            printf("%012.3fms: p%d leaves Q2, time in Q2 = %.3fms\n", new->q2_out_time, new->packet_id, new->q2_out_time - new->q2_in_time);
            // record service start time
            new->service_st_time = getTime(startTime);
            printf("%012.3fms: p%d begins service at S%d, requesting %.3fms of service\n", new->service_st_time, new->packet_id, serverID, new->service_time);      
        } else {
            pthread_mutex_unlock(&mutex);
            break;
        }
    /////////////////////////////////////////////////////////////////////
        // unlocked
        pthread_mutex_unlock(&mutex);

        if (lambda >= MAX_MS_TIME && !isFile) {
        } else {
            usleep(1000 * new->service_time);            
        }

        // locked
        pthread_mutex_unlock(&mutex);
        // record exiting server time
        new->service_ex_time = getTime(startTime);
        printf("%012.3fms: p%d departs from S%d, service time = %.3fms, time in system = %.3fms\n", new->service_ex_time, new->packet_id, serverID, new->service_ex_time - new->service_st_time, new->service_ex_time - new->arrival_time);

        analyzeService(new, serverID);  
        // unlocked
        pthread_mutex_unlock(&mutex);
    
    }
    isRunning = false;
    pthread_exit(NULL);
}

/* Catch SIGINT <Ctrl+C> and stop running all other threads */
void *signalCatching(void *arg)
{
    // while (1) {
        int sig;
        sigwait(&set, &sig);

        // locked
        pthread_mutex_lock(&mutex);
        printf("\n%012.3fms: SIGINT caught, no new packets or tokens will be allowed\n", getTime(startTime));
        isRunning = false;
        isRunningPacket = false;
        isRunningToken = false;
         // broadcast
        pthread_cond_broadcast(&cv);
        // cancel packet and token threads
        pthread_cancel(packet_thr);
        pthread_cancel(token_thr);

        // a packet is removed when it's in Q1 because <Ctrl+C> is pressed by the user
        while (!My402ListEmpty(&q1)) {
            Packet *curr_q1 = (Packet *)(My402ListFirst(&q1)->obj);
            My402ListUnlink(&q1, My402ListFirst(&q1));
            printf("%012.3fms: p%d removed from Q1\n", getTime(startTime), curr_q1->packet_id);
        }
        // a packet is removed when it's in Q1 because <Ctrl+C> is pressed by the user
        while (!My402ListEmpty(&q2)) {
            Packet *curr_q2 = (Packet *)(My402ListFirst(&q2)->obj);
            My402ListUnlink(&q2, My402ListFirst(&q2));
            printf("%012.3fms: p%d removed from Q2\n", getTime(startTime), curr_q2->packet_id);
        }
 
        // unlocked
        pthread_mutex_unlock(&mutex);
        pthread_exit(NULL);
    // }
    // return 0;
}

/* Create packet thread, token thread, two server thread and signal catching thread */
void createThreads()
{
    // create packet thread
    if (pthread_create(&packet_thr, 0, packetThread, NULL)) {
        fprintf(stderr, "Error: Fail to create packect thread!\n");
        exit(0);
    }
    // create token thread
    if (pthread_create(&token_thr, 0, tokenThread, NULL)) {
        fprintf(stderr, "Error: Fail to create token thread!\n");
        exit(0);
    }
    // create server_1 thread
    if (pthread_create(&s1_thr, 0, serverThread, (void *)1)) {
        fprintf(stderr, "Error: Fail to create server_1 thread!\n");
        exit(0);
    }
    // create server_2 thread
    if (pthread_create(&s2_thr, 0, serverThread, (void *)2)) {
        fprintf(stderr, "Error: Fail to create server_2 thread!\n");
        exit(0);
    }
    // create signal handler thread
    if (pthread_create(&signal_thr, 0, signalCatching, NULL)) {
        fprintf(stderr, "Error: Fail to create signal handler thread!\n"); 
        exit(0);
    }
}

/* Do emulation */
void emulate() 
{
    // ready to start your emulation
    gettimeofday(&startTime, NULL);
    printf("%012.3fms: emulation begins\n", 00000000.000);
    // create threads
    createThreads();
    // wait
    pthread_join(packet_thr, NULL);
    pthread_join(token_thr, NULL);
    pthread_join(s1_thr, NULL);
    pthread_join(s2_thr, NULL);
    // ready to end your emulation
    emulation_time = getTime(startTime);
    printf("%012.3fms: emulation ends\n\n", emulation_time);
    if (isFile) {
        fclose(fp);
    }
}

/* Print all other statistics after emulation */
void printStatistics()
{
    printf("Statistics:\n\n");

    // average packet inter-arrival time
    if(packets_total_num == 0) {
		printf("\taverage packet inter-arrival time = (N/A, no packet arrived)\n");
	} else {    
        printf("\taverage packet inter-arrival time = %.6g\n", total_interval_time / packets_total_num /1000.0);
    }
    // average packet service time
    if (packets_finished == 0) {
        printf("\taverage packet service time = (N/A, no packet served)\n\n");
    } else {
        printf("\taverage packet service time = %.6g\n\n", total_service_time / packets_finished / 1000.0);
    }

    // average number of packets in Q1, Q2, S1, S2
    printf("\taverage number of packets in Q1 = %.6g\n", (total_q1_time / emulation_time));
    printf("\taverage number of packets in Q2 = %.6g\n", (total_q2_time / emulation_time));
    printf("\taverage number of packets at S1 = %.6g\n", (total_s1_time / emulation_time));
    printf("\taverage number of packets at S2 = %.6g\n\n", (total_s2_time / emulation_time));

    // average time a packet spent in system and standard deviation time
    if (packets_finished == 0) {
		printf("\taverage time a packet spent in system = (N/A, no packet served)\n");
		printf("\tstandard deviation for time spent in system = (N/A, no packet served)\n\n");        
    } else {
		printf("\taverage time a packet spent in system = %.8g\n", total_system_time / packets_finished / 1000.0);
		printf("\tstandard deviation for time spent in system = %.8g\n\n", sqrt((total_system_time_sq / packets_finished) - pow(total_system_time / packets_finished, 2)) / 1000.0);
    }
    // token drop probability
    if (token_id == 0) {
        printf("\ttoken drop probability = %.6g, no tokens arrived at this facility\n", 1.0 * tokens_dropped / token_id);
    } else {
        printf("\ttoken drop probability = %.6g\n", 1.0 * tokens_dropped / token_id);
    }

    // packet drop probability
    if (packets_total_num == 0) {
        printf("\tpacket drop probability = %.6g, no packets arrived at this facility\n", 1.0 * packets_dropped / packets_total_num);
    } else {
        printf("\tpacket drop probability = %.6g\n\n", 1.0 * packets_dropped / packets_total_num);
    }

}

int main(int argc, char *argv[])
{
    // parse command line
    readCmd(argc, argv);
    // check if tsfile is in right format
    checkFile();
    // initialize signal, pthread and lists
    initialize();
    // print emulation parameters
    printEmulationPara();
    // start emulation
    emulate();
    // print data analysis
    printStatistics();

    return 0;
}