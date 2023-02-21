#pragma once

typedef enum HT_ErrorCode {
  HT_OK,
  HT_ERROR
} HT_ErrorCode;

typedef struct {  //μπορειτε να αλλαξετε τη δομη συμφωνα  με τις ανάγκες σας
	char surname[20];
	char city[20];
	int oldTupleId; // η παλια θέση της εγγραφής πριν την εισαγωγή της νέας
	int newTupleId; // η νέα θέση της εγγραφής που μετακινήθηκε μετα την εισαγωγή της νέας εγγραφής 
	
} UpdateRecordArray;

typedef struct Record {
	int id;
	char name[15];
	char surname[20];
	char city[20];
} Record;

typedef struct info{
  char file_name[20];
  int index;
}Info;

#define INT_SIZE 5
#define NAME_SIZE 15
#define SURNAME_SIZE 20
#define CITY_SIZE 20
#define FILE_NAME_SIZE 20
#define ATTR_NAME_SIZE 20
#define RECORD_SIZE (INT_SIZE + NAME_SIZE + SURNAME_SIZE + CITY_SIZE)
#define SECONDARY_RECORD_SIZE (ATTR_NAME_SIZE + INT_SIZE)
#define MAX_RECORDS 8
#define MAX_SECONDARY_RECORDS 16

#define MAX_OPEN_FILES 20
#define INT_SIZE 5
#define DIR_BLOCKS 1
#define DIR_BEGINS 1
#define DIR_ENDS 1
#define DIR_MAX_LEN 32
#define DIR_MAX_KEYS 32

//Δηλώσεις βοηθητικών συναρτήσεων

int get_int(int start, int len, char* src);
char* get_string(int start, int len, char* src);
void print_block(char* data);
void print_char(int start, int len, char* string);
char* itos(int number);
char* toBinary(int number, int len);
void print_hash_table(char* dir, int depth);
char* hashFunction(int id, int depth);
int get_last_bucket(int index);
void make_dir(int depth, char* dir);
void expand_dict(int new_depth, char* dir, int overflowed_bucket, int last);
int get_bucket(char* hash_value, int depth, char* dict);
int store_record(Record record, char* data);
void split(int index, int old_pointer, char* bucket, Record record, char* dir, int* tuple_id,UpdateRecordArray updateArray[MAX_RECORDS]);
void pointers_adapt(int new_depth, char* dir, int overflowed_bucket, int last);
void dirty_unpin_all(int indexDesc);

/*
 * Η συνάρτηση HT_Init χρησιμοποιείται για την αρχικοποίηση κάποιον δομών που μπορεί να χρειαστείτε. 
 * Σε περίπτωση που εκτελεστεί επιτυχώς, επιστρέφεται HT_OK, ενώ σε διαφορετική περίπτωση κωδικός λάθους.
 */
HT_ErrorCode HT_Init();

/*
 * Η συνάρτηση HT_CreateIndex χρησιμοποιείται για τη δημιουργία και κατάλληλη αρχικοποίηση ενός άδειου αρχείου κατακερματισμού με όνομα fileName. 
 * Στην περίπτωση που το αρχείο υπάρχει ήδη, τότε επιστρέφεται ένας κωδικός λάθους. 
 * Σε περίπτωση που εκτελεστεί επιτυχώς επιστρέφεται HΤ_OK, ενώ σε διαφορετική περίπτωση κωδικός λάθους.
 */
HT_ErrorCode HT_CreateIndex(
	const char *fileName,		/* όνομααρχείου */
	int depth
	);


/*
 * Η ρουτίνα αυτή ανοίγει το αρχείο με όνομα fileName. 
 * Εάν το αρχείο ανοιχτεί κανονικά, η ρουτίνα επιστρέφει HT_OK, ενώ σε διαφορετική περίπτωση κωδικός λάθους.
 */
HT_ErrorCode HT_OpenIndex(
	const char *fileName, 		/* όνομα αρχείου */
  int *indexDesc            /* θέση στον πίνακα με τα ανοιχτά αρχεία  που επιστρέφεται */
	);

/*
 * Η ρουτίνα αυτή κλείνει το αρχείο του οποίου οι πληροφορίες βρίσκονται στην θέση indexDesc του πίνακα ανοιχτών αρχείων.
 * Επίσης σβήνει την καταχώρηση που αντιστοιχεί στο αρχείο αυτό στον πίνακα ανοιχτών αρχείων. 
 * Η συνάρτηση επιστρέφει ΗΤ_OK εάν το αρχείο κλείσει επιτυχώς, ενώ σε διαφορετική σε περίπτωση κωδικός λάθους.
 */
HT_ErrorCode HT_CloseFile(
	int indexDesc 		/* θέση στον πίνακα με τα ανοιχτά αρχεία */
	);

/*
 * Η συνάρτηση HT_InsertEntry χρησιμοποιείται για την εισαγωγή μίας εγγραφής στο αρχείο κατακερματισμού. 
 * Οι πληροφορίες που αφορούν το αρχείο βρίσκονται στον πίνακα ανοιχτών αρχείων, ενώ η εγγραφή προς εισαγωγή προσδιορίζεται από τη δομή record. 
 * Σε περίπτωση που εκτελεστεί επιτυχώς επιστρέφεται HT_OK, ενώ σε διαφορετική περίπτωση κάποιος κωδικός λάθους.
 */
HT_ErrorCode HT_InsertEntry(
	int indexDesc,	/* θέση στον πίνακα με τα ανοιχτά αρχεία */
	Record record,		/* δομή που προσδιορίζει την εγγραφή */
	int* tupleId,
	UpdateRecordArray* updateArray
	);

/*
 * Η συνάρτηση HΤ_PrintAllEntries χρησιμοποιείται για την εκτύπωση όλων των εγγραφών που το record.id έχει τιμή id. 
 * Αν το id είναι NULL τότε θα εκτυπώνει όλες τις εγγραφές του αρχείου κατακερματισμού. 
 * Σε περίπτωση που εκτελεστεί επιτυχώς επιστρέφεται HP_OK, ενώ σε διαφορετική περίπτωση κάποιος κωδικός λάθους.
 */
HT_ErrorCode HT_PrintAllEntries(
	int indexDesc,	/* θέση στον πίνακα με τα ανοιχτά αρχεία */
	int *id 				/* τιμή του πεδίου κλειδιού προς αναζήτηση */
	);

HT_ErrorCode HashStatistics(const char *filename);