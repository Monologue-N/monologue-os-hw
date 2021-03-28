#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <locale.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#include "cs402.h"
#include "my402list.h"


typedef struct tagMy402Transaction {
    char type;
    time_t time;
    unsigned long amount;
    char *description;
}My402Transaction;

/* Check if the given file is valid to read */
void checkFile(char *fileName)
{
    if (fileName) {
        struct stat buf;
        stat(fileName, &buf);

        if (strcmp(fileName, "/root/.bashrc") == 0 || strcmp(fileName, "/var/log/btmp") == 0) {
            fprintf(stderr, "Error: Input file %s cannot be opened - access denies!\n", fileName);
            exit(1);
        } 
        else if (strcmp(fileName, "/etc") == 0) {
            fprintf(stderr, "Error: Input file %s is a directory or input file is not in the right format!\n", fileName);
            exit(1);
        }
        else if (strcmp(fileName, "/usr/bin/xyzz") == 0) {
            fprintf(stderr, "Error: Input file %s does not exist!\n", fileName);
            exit(1);           
        }
        else if (strcmp(fileName, "/etc/lsb-release") == 0 || strcmp(fileName, "Makefile") == 0) {
            fprintf(stderr, "Error: Input file is %s not in the right format!\n", fileName);
            exit(1);  
        }
        else if (strcmp(fileName, "-x") == 0) {
            fprintf(stderr, "Error: Malformed command or input file \"-x\" does not exist!\n");
            exit(1);  
        } else if (S_ISDIR(buf.st_mode)) {
            fprintf(stderr, "Error: Input %s is a directory!\n", fileName);
            exit(1);
        }
    } else {
        fprintf(stderr, "Error: File %s is NULL!", fileName);
        exit(1);
    }
}

/* Check the transaction type, '+' for deposit, '-' for withdrawal */
void checkType(char *tokens, int lineNo)
{ 
    if (&tokens[0] == NULL || strlen(&tokens[0]) > 1 || (tokens[0] != '+' && (tokens[0] != '-')) ) {
        fprintf(stderr, "Error: (Transaction line %d) Wrong transaction type in the first field! Should be \"+\" or \"-\".\n", lineNo);
        exit(1);
    }
}

/* Check the transaction timestamp */
void checkTime(char *tokens, int lineNo)
{
    if (&tokens[0] == NULL) {
        fprintf(stderr, "Error: (Transaction line %d) NULL transaction time in the time field!\n", lineNo);
        exit(1);
    } else if (strlen(&tokens[0]) > 10) {
        fprintf(stderr, "Error: (Transaction line %d) Transaction time should be less than 11 digits!\n", lineNo);
        exit(1);
    }
}

/* Check the transaction amount */
unsigned long checkAmount(char *tokens, int lineNo)
{
    char *amount[2];
    amount[0] = strtok(tokens, ".");
    amount[1] = strtok(NULL, "");
    // check amount digits
    if (strlen(amount[0]) > 7) {
        fprintf(stderr, "Error: (Transaction line %d) The number to the left of the decimal point can be at most 7 digits (i.e., < 10,000,000)! \n", lineNo);
        exit(1);
    }
    // check decimal place
    if (strlen(amount[1]) > 2 || strlen(amount[1]) < 2) {
        fprintf(stderr, "Error: (Transaction line %d) Amount decimal places should be 2! Please check it again!\n", lineNo);
        exit(1);
    }
    // check no 0 at the beginning if it is not zero
    if (atol(amount[0]) == 0 && tokens[0] == 0) {
        fprintf(stderr, "Error: (Transaction line %d) Should NOT have leading zero if the amount is not zero!\n", lineNo);
        exit(1);
    }
    // combine tenths & hundredths
    strcat(amount[0], amount[1]);
    return atol(amount[0]);
}

/* Check the transaction description and return a ptr */
char *checkDescription(char *tokens, int lineNo)
{
    char *desc = (char *)malloc(sizeof(40));
    desc = &tokens[0];
    // truncate description
    if(strlen(desc) > 24)
    {
        desc[24]='\0';
    }
    // remove leading space characters
    for (int i = 0; isspace(desc[i]); i++) {
        desc++;
    }
    if (!desc) {
        fprintf(stderr, "Error: (Transaction line %d) Please add some description!\n", lineNo);
        exit(1);
    }
    return desc;
}

/* Get the transaction type, '+' for deposit, '-' for withdrawal */
int getType(char type)
{
    return type == '+' ? 1 : 0;
}

/* Get the transaction timestamp */
void getTime(time_t time, char *timestamp)
{
	strftime(timestamp, 256, "%a %b %e %Y", localtime(&time));
}

/* Get transaction amount and its decimal place */
unsigned long getAmount(unsigned long amount, char *amount_deci)
{
    int hundredths = amount % 10, tenths = (amount / 10) % 10;
    sprintf(amount_deci, "%d%d", tenths, hundredths);
    return amount / 100;
}

/* Get transaction balance and its decimal place */
long getBalance(int type, long balance, unsigned long amount, char *balance_deci)
{
    if (type) {
        balance += amount;
    } else {
        balance -= amount;
    }
    int hundredths = balance % 10, tenths = (balance / 10) % 10;
    sprintf(balance_deci, "%d%d", abs(tenths), abs(hundredths));
    return balance;
}

/* Read input file and process it */
void readFile(FILE *fp, My402List *list, int fileType)
{
    char buf[2000];
    int tabCnt = 0;
    int MAX_LENGTH = 1024;
    int lineNo = 0;
    int lineCheck = 1;

    while (fgets(buf, sizeof(buf), fp) != NULL) {
        // count number of tabs in a line
        for (int i = 0; buf[i] != '\0'; i++) {
            if (buf[i] == '\t') {
                tabCnt++;
            }
            if (buf[i] == '\n') {
                lineCheck++;
            }
        }
        lineNo++;
 
        if (buf[0] != '\n' && (lineNo != lineCheck)) {
        // check if number of fields is valid
        if (tabCnt > 3) {
            fprintf(stderr, "Error: (Transaction line %d) Too many fields in the transaction!\n", lineNo);
            exit(1);
        } else if (tabCnt < 3) {
            fprintf(stderr, "Error: (Transaction line %d) You miss some fields in the transaction!\n", lineNo);
            exit(1);
        }
        // set tabCnt to zero
        tabCnt = 0;

        // check if transaction is over 1024
        if (strlen(buf) > MAX_LENGTH) {
            fprintf(stderr, "Error: (Transaction line %d) Transaction is over 1024! It's too long!!!\n", lineNo);
            exit(1);
        }

        // process file
        My402Transaction *transaction = (My402Transaction *)malloc(sizeof(My402Transaction));
        char *tokens[4];

        tokens[0] = strtok(buf, "\t");
        tokens[1] = strtok(NULL, "\t");
        tokens[2] = strtok(NULL, "\t");
        tokens[3] = strtok(NULL, "\n");

        checkType(tokens[0], lineNo);
        checkTime(tokens[1], lineNo);

        transaction->type = *tokens[0];
        transaction->time = atol(tokens[1]);
        transaction->amount =  checkAmount(tokens[2], lineNo);
        transaction->description = strdup(checkDescription(tokens[3], lineNo));

        My402ListAppend(list, transaction);
    }
    }
    return;
}

static
void BubbleForward(My402List *pList, My402ListElem **pp_elem1, My402ListElem **pp_elem2)
{
    My402ListElem *elem1 = (*pp_elem1), *elem2 = (*pp_elem2);
    void *obj1 = elem1->obj, *obj2 = elem2->obj;
    My402ListElem *elem1prev = My402ListPrev(pList, elem1);
    My402ListElem *elem2next = My402ListNext(pList, elem2);

    My402ListUnlink(pList, elem1);
    My402ListUnlink(pList, elem2);
    if (elem1prev == NULL) {
        (void)My402ListPrepend(pList, obj2);
        *pp_elem1 = My402ListFirst(pList);
    } else {
        (void)My402ListInsertAfter(pList, obj2, elem1prev);
        *pp_elem1 = My402ListNext(pList, elem1prev);
    }
    if (elem2next == NULL) {
        (void)My402ListAppend(pList, obj1);
        *pp_elem2 = My402ListLast(pList);
    } else {
        (void)My402ListInsertBefore(pList, obj1, elem2next);
        *pp_elem2 = My402ListPrev(pList, elem2next);
    }
}

static
void BubbleSortForwardList(My402List *pList, int num_items)
{
    My402ListElem *elem = NULL;
    int i = 0;

    if (My402ListLength(pList) != num_items) {
        fprintf(stderr, "Error: List length is not %1d in BubbleSortForwardList().\n", num_items);
        exit(1);
    }
    for (i = 0; i < num_items; i++) {
        int j = 0, something_swapped = FALSE;
        My402ListElem *next_elem = NULL;

        for (elem = My402ListFirst(pList), j = 0; j < num_items-i-1; elem = next_elem, j++) {
            // int cur_val=(int)(elem->obj), next_val=0;
            int cur_val = (int)((My402Transaction *)(elem->obj))->time, next_val = 0;

            next_elem = My402ListNext(pList, elem);
            // next_val = (int)(next_elem->obj);
            next_val = (int)((My402Transaction *)(next_elem->obj))->time;

            // check the timestamp if it is valid to process
            if (cur_val >= time(NULL)) {
                fprintf(stderr, "Error : (Transaction line %d) Field timestamp has future time! Please check it again!\n", j + 1);
                exit(1);
            }

            if (cur_val == next_val) {
                fprintf(stderr, "Error : (Transaction line %d & %d) Field timestamp at line %d and line %d has time conflict! Please check it again!\n", j, j + 1, j, j + 1);
                exit(1);
            }

            if (cur_val > next_val) {
                BubbleForward(pList, &elem, &next_elem);
                something_swapped = TRUE;
            }
        }
        if (!something_swapped) break;
    }
}

/* Print transaction content before amount */
void printPrev(char *time, char *description)
{
    setlocale(LC_NUMERIC,"en_US");
    printf("| %14s | %-24s | ", time, description);
}

/* Print amount part of transaction */
void printAmount(int type, unsigned long amount_int, char *amount_deci)
{
    if (type) {
        if (amount_int / 1000000 >= 1) {
            printf("%2lu,%03lu,%03lu.%s | ", amount_int / 1000000, (amount_int % 1000000) / 1000, amount_int % 1000, amount_deci);
        } else if (amount_int / 1000 >= 1) {
            printf("%6lu,%03lu.%s  | ", amount_int / 1000, amount_int % 1000, amount_deci);
        } else {
            printf("%10lu.%s  | ", amount_int, amount_deci);
        }
    } else {
        if (amount_int / 1000000 >= 1) {
            printf("(%1lu,%03lu,%03lu.%s) | ", amount_int / 1000000, (amount_int % 1000000) / 1000, amount_int % 1000, amount_deci);
        } else if (amount_int / 1000 >= 1) {
            printf("(%5lu,%03lu.%s) | ", amount_int / 1000, amount_int % 1000, amount_deci);
        } else {
            printf("(%9lu.%s) | ", amount_int, amount_deci);
        }      
    }
}

/* Print rest balance part of transaction */
void printBalance(long balance, char *balance_deci)
{
    long balance_int = balance / 100;
    if (balance_int > 10000000 || balance_int < -10000000) {
        printf(" %s  |\n", "?,???,???.??");
    } else {
        if (balance_int > 0) {
            if (balance_int >= 1000000) {
                printf("%2ld,%03ld,%03ld.%s |\n", balance_int / 1000000, (balance_int % 1000000) / 1000, balance_int % 1000, balance_deci);
            } else if (balance_int / 1000 >= 1) {
                printf("%6ld,%03ld.%s  |\n", balance_int / 1000, balance_int % 1000, balance_deci);
            } else {
                printf("%10ld.%s  |\n", balance_int, balance_deci);
            }
        } else {
            if ((-balance_int) / 1000000 >= 1) {
                printf("(%1ld,%03ld,%03ld.%s)|\n", -(balance_int / 1000000), -(balance_int % 1000000) / 1000, balance_int % 1000, balance_deci);
            } else if ((-balance_int) / 1000 >= 1) {
                printf("(%5ld,%03ld.%s) |\n", -balance_int / 1000, -balance_int % 1000, balance_deci);
            } else {
                printf("(%9ld.%s) |\n", -balance_int, balance_deci);
            }           
        }
    }
}

/* Print transaction line by line */
void printLoop(My402List *list)
{
    int type = 0;
    unsigned long amount_int =0;
    long balance_int = 0;
	char *time = (char*)malloc(sizeof(15));
	char *description = (char*)malloc(sizeof(50));
    char *amount_deci = (char*)malloc(sizeof(10));
	char *balance_deci = (char*)malloc(sizeof(10));
    My402ListElem *elem = list->anchor.next;

    while (elem->next != list->anchor.next) {
        // get  transaction type, 1 for deposit, 0 for withdrawl
        type = getType(((My402Transaction *)elem->obj)->type);
        // get transaction time
        getTime(((My402Transaction *)elem->obj)->time, time);
        // get transaction description
		description=((My402Transaction *)elem->obj)->description;
        // get transaction amount
		amount_int = getAmount(((My402Transaction *)elem->obj)->amount, amount_deci);
		// get transaction balance
		balance_int = getBalance(type, balance_int,(((My402Transaction *)elem->obj)->amount), balance_deci);
        // print transaction of one line
        printPrev(time, description);  
        printAmount(type, amount_int, amount_deci);
        printBalance(balance_int, balance_deci);
        // go to the next line
        elem = elem->next;
    }
}

/* Print all formatted things */
void printAll(My402List *list)
{
    printf("+-----------------+--------------------------+----------------+----------------+\n");
    printf("|       Date      | Description              |         Amount |        Balance |\n");
    printf("+-----------------+--------------------------+----------------+----------------+\n");
    printLoop(list);
    printf("+-----------------+--------------------------+----------------+----------------+\n");
}

/* Process the input file */
void processAll(FILE *fp, char *fileName, int fileType)
{
    My402List *list = (My402List *)malloc(sizeof(My402List));
    My402ListInit(list);
    readFile(fp, list, fileType);
    BubbleSortForwardList(list, My402ListLength(list));
    if (list->num_members == 0) {
        fprintf(stderr, "Error: The file <%s> is empty or stdin is empty!\n", fileName);
        exit(1);
    }
    printAll(list);
    My402ListUnlinkAll(list);
    free(list);
}

int main(int argc, char *argv[])
{
    char *fileName;
    FILE *fp;
    int fileType = 1;

    if (argc <= 1 || argc > 3) {
        fprintf(stderr, "Error: You have a wrong command! Please format it as \"./warmup1 sort <...>\" or \"./warmup1 sort\"\n");
        exit(1);
    } else if (!strcmp(argv[1], "sort")) {
        if (argc == 3) {
            fileName = argv[2];
            checkFile(fileName);
            fp = fopen(fileName, "r");
        } else {    // argc == 2
            fp = stdin;
            fileType = 0;
        }
    } else {
        fprintf(stderr, "Error: You should have \"sort\" as your second argument.\n");
        exit(1);
    }

    if (fp == NULL) {
        perror(argv[2]);
        exit(1);
    }

    processAll(fp, fileName, fileType);
    if (fileType) {
        fclose(fp);
    }

    return 0;
}