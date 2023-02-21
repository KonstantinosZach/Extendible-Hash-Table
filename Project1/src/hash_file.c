#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "bf.h"
#include "hash_file.h"
#define MAX_OPEN_FILES 20
#define INT_SIZE 5
#define DIR_BLOCKS 1
#define DIR_BEGINS 1
#define DIR_ENDS 1
#define DIR_MAX_LEN 32
#define DIR_MAX_KEYS 32

#define CALL_BF(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {         \
    BF_PrintError(code);    \
    return HP_ERROR;        \
  }                         \
}

//make int to string must free after
char* itos(int number){
  char* number_str = malloc(sizeof(char)*INT_SIZE);
  sprintf(number_str, "%d", number);
  number_str[INT_SIZE-1] = '\0';
  return number_str;
}

//prints the string from start to len
void print_char(int start, int len, char* string){
  printf("Print string: %.*s\n", len, string + start);
}

//binary representation of an int with specific lenght must free after
char* toBinary(int number, int len)
{
    char* binary = (char*)malloc(sizeof(char) * len);
    int k = 0;
    for (unsigned i = (1 << len - 1); i > 0; i = i / 2) {
        binary[k++] = (number & i) ? '1' : '0';
    }
    binary[k] = '\0';
    return binary;
}

//pritns the directory -> keys with pointers
void print_hash_table(char* dir, int depth){
  int len = pow(2,depth);
  unsigned long offset = 0;

  //Δεν αφήνουμε το dir να πιάσει πάνω από DIR_MAX_KEYS
  if(len > DIR_MAX_KEYS){
    printf("The lenght of directory is exceeded\n");
    exit(1);
  }

  //Διασχίζουμε το directory και παιρνουμε τα keys και τους pointers
  for(int i=0; i< len; i++){

    int key = get_int(offset, INT_SIZE, dir);
    char* binary_key = toBinary(key,depth);
    printf("key: %s ",binary_key);
    free(binary_key);

    offset += sizeof(char)*INT_SIZE;
    int pointer = get_int(offset, INT_SIZE, dir);
    printf("pointer: %d\n",pointer);

    offset += sizeof(char)*INT_SIZE;
  }
}

//our hash function
char* hashFunction(int id, int depth){
  char* binary = toBinary(id, depth);
  int new_id = (int) strtol(binary, NULL, 2);
  free(binary);
  return toBinary(new_id,depth);
}

//gets a specific part from src string must free after
char* get_string(int start, int len, char* src){
  char* substring = malloc(sizeof(char)*len);
  strncpy(substring, &src[start], len);
  substring[len - 1] = '\0';
  return substring;
}

//from a string we get the int from specific position
int get_int(int start, int len, char* src){
  char* data = get_string(start, len, src);
  int data_int = atoi(data);
  free(data);
  return data_int;
}

//returns the position of last bucket
int get_last_bucket(int index){
  int num;
  BF_GetBlockCounter(index,&num);
  return num;
}

//makes the the original directory(hash table)
void make_dir(int depth, char* dir){
  unsigned long offset = 0;
  int len = pow(2,depth);

  //Δεν αφήνουμε το dir να πιάσει πάνω από DIR_MAX_KEYS
  if(len > DIR_MAX_KEYS){
    printf("The lenght of directory is exceeded\n");
    exit(1);
  }

  for (int i=0; i<len; i++){
    char* key = itos(i);
    memcpy(dir + offset, key, strlen(key));
    offset += sizeof(char)*INT_SIZE;

    char* pointer = itos(i+DIR_BLOCKS+1);           //τα buckets ξεκινανε μετά από το info και directory block
    memcpy(dir + offset, pointer, strlen(pointer));
    offset += sizeof(char)*INT_SIZE;

    free(key);
    free(pointer);
  }
}

//expands the directory(hash table) when we encounter an overflow
void expand_dict(int new_depth, char* dir, int overflowed_bucket, int last){
  int len = pow(2,new_depth);

  if(len > DIR_MAX_KEYS){
    printf("The lenght of directory is exceeded\n");
    exit(1);
  }

  int old_size = pow(2,new_depth-1);
  int temp_keys[old_size];
  int temp_pointers[old_size];
  int dict_offset = old_size;
  int bit_offset = 0;

  //αποθηκεύουμε τα κλειδία και τους pointers του dir που
  //υπαρχoυν ήδη και τα χρησιμοποιούμε για να φτιάξουμε το καινούργιο μέρος του dir
  for (int i=0; i< len; i++){
    if(i < old_size){
      //εδω αποθηκευουμε το "παλιό" μερος του dir σε temp πινακες
      int key = get_int(bit_offset,INT_SIZE,dir);
      temp_keys[i] = key;
      bit_offset += sizeof(char)*INT_SIZE;

      int pointer = get_int(bit_offset,INT_SIZE,dir);
      temp_pointers[i] = pointer;
      bit_offset += sizeof(char)*INT_SIZE;
    }
    else{
      //και εδω χρησιμοποιούμε αυτους τους πινακες για να φτιάξουμε το extend dir (το καινούργιο μέρος)
        int temp_key = temp_keys[i-dict_offset] + old_size;
        char* key = itos(temp_key);
        char* pointer = itos(temp_pointers[i-dict_offset]);

        memcpy(dir+bit_offset,key,strlen(key));
        bit_offset += sizeof(char)*INT_SIZE;

        //αν βρουμε τον overflowed bucket ως pointer τοτε δεν τον αποθηκευουμε
        //αλλά στη θέση του μπαίνει το bucket που δημιουργήθηκε τελευταιό
        if(atoi(pointer) == overflowed_bucket){
          char* new_bucket = itos(last);
          memcpy(dir+bit_offset,new_bucket,strlen(new_bucket));
          bit_offset += sizeof(char)*INT_SIZE;
          free(new_bucket);
        }
        else{
          memcpy(dir+bit_offset,pointer,strlen(pointer));
          bit_offset += sizeof(char)*INT_SIZE;
        }

        free(key);
        free(pointer);
    }
  }

}

//we get the bucket with hash value from the directory
int get_bucket(char* hash_value, int depth, char* dict){

  int len = pow(2,depth);
  unsigned long offset = 0;

  //Διασχίζουμε όλο το dir και βρίσκουμε το κλειδι
  //που είναι ιδιο με το hash value και επιστρέφουμε τον αριθμό του bucket
  for(int i=0; i<len; i++){

    int key = get_int(offset, INT_SIZE, dict);
    char* binary_key = toBinary(key,depth);

    offset += sizeof(char)*INT_SIZE;
    int pointer = get_int(offset, INT_SIZE, dict);

    offset += sizeof(char)*INT_SIZE;

    if(strcmp(binary_key,hash_value) == 0){
      return pointer;
    }
    free(binary_key);
  }
  //άμα δεν πετυχει επιστρέφουμε -1
  return -1;
}

//stores the record to the given bucket
int store_record(Record record, char* data){

  //Σιγουρα γραφουμε μετα το ΙΝΤ_ΜΑΧ => local depth
  int count = get_int(INT_SIZE, INT_SIZE, data);

  if(count == MAX_RECORDS){
    //Υπερχειλιση
    return -1;
  }

  //περνάμε στο bucket το record χρησιμοποιώντας το offset κατάλληλα
  char* id_string = itos(record.id);
  unsigned long offset = sizeof(char)*INT_SIZE*2 + sizeof(char)*RECORD_SIZE*count;
  memcpy(data + offset, id_string, strlen(id_string));
  offset += sizeof(char)*INT_SIZE;
  memcpy(data + offset, record.name, strlen(record.name));
  offset += sizeof(char)*NAME_SIZE;
  memcpy(data + offset, record.surname, strlen(record.surname));
  offset += sizeof(char)*SURNAME_SIZE;
  memcpy(data + offset, record.city, strlen(record.city));


  //πρεπει να ενημερώσουμε και να αποθηκευσουμε το count
  count++;
  char* count_string = itos(count);
  memcpy(data + sizeof(char)*INT_SIZE, count_string, strlen(count_string));
  free(count_string);
  free(id_string);
  return 0;
}

//we split the overflowed bucket
void split(int index, char* bucket, Record record, char* dir){
  BF_Block *block;
  BF_Block_Init(&block);
  BF_AllocateBlock(index, block);

  //Το καινούργιο block
  char* new_block_data = BF_Block_GetData(block);
  //Βαζουμε local depth 0 για να μπορούμε να το ξεχωρισουμε απο το overlowed bucket
  memcpy(new_block_data,"0", strlen("0"));
  memcpy(new_block_data + sizeof(char)*INT_SIZE, "0", strlen("0"));

  int counter_old = 0;  //ποσα records μπαίνουν στον overflowed bucket
  int counter_new = 0;  //ποσα records μπαίνουν στον κανουργιο bucket

  char* local_depth = get_string(0,INT_SIZE,bucket);      //παιρνουμε το local depth του overflowed bucket

  unsigned long bucket_offset = sizeof(char)*INT_SIZE*2;  //και κραταμε τα 2 offsets που χρειαζομαστε
  unsigned long old_offset = INT_SIZE;

  //Κανουμε MAX_RECORDS γιατί έχει γεμισει το bucket με records
  for(int rec=0; rec<MAX_RECORDS; rec++){

    //Παίρνουμε το id των records
    int id = get_int(bucket_offset,INT_SIZE,bucket);
    bucket_offset += INT_SIZE;

    //το Hash-αρουμε και παιρνουμε τον αριθμό του bucket που θα δειχνει σε αυτο το record
    //(το local depth είναι ήδη αυξημένο)
    char* hash_value = hashFunction(id,atoi(local_depth));
    int pointer = get_bucket(hash_value,atoi(local_depth),dir);
    free(hash_value);
    
    //Και παιρνουμε το bucket στο οποιο θα αποθηκευσουμε το record
    //από το οποιο παίρνουμε το local depth
    //Aν το local depth ειναι 0 τοτε θα βαλουμε το record στο καινουργιο bucket αν οχι τοτε στο overflowed bucket
    BF_GetBlock(index, pointer, block);
    char* check_data = BF_Block_GetData(block);
    int check_depth = get_int(0,INT_SIZE,check_data);

    //παιρνουμε τα υπολοιπα στοιχεία του record και αλλάζουμε καταλληλα το offset
    char* string_id = itos(id);

    char* name = get_string(bucket_offset,NAME_SIZE,bucket);
    bucket_offset += NAME_SIZE;

    char* surname = get_string(bucket_offset,SURNAME_SIZE,bucket);
    bucket_offset += SURNAME_SIZE;
    
    char* city = get_string(bucket_offset,CITY_SIZE,bucket);
    bucket_offset += CITY_SIZE;

    if(check_depth == 0){
      unsigned long new_offset = INT_SIZE;
      counter_new++;

      //Αποθηκευουμε το καινουργιο counter κάθε φορά
      char* string_counter = itos(counter_new);
      memcpy(new_block_data +sizeof(char)*new_offset,string_counter,strlen(string_counter));
      free(string_counter);

      new_offset += INT_SIZE + sizeof(char)*RECORD_SIZE*(counter_new-1);    //προχωραμε μπροστά όσα records εχουεμ ήδη αποθηκεύσει

      //και αποθηκευουme μετα τα υπολοιπα στοιχεία
      memcpy(new_block_data + new_offset,string_id,strlen(string_id));

      new_offset += INT_SIZE;
      memcpy(new_block_data + new_offset,name,strlen(name));

      new_offset += NAME_SIZE;
      memcpy(new_block_data + new_offset,surname,strlen(surname));

      new_offset += SURNAME_SIZE;
      memcpy(new_block_data + new_offset,city,strlen(city));

    }
    else{
      //αλλιως βρισκομαστε στην περιπτωση που αποθηκευουμε το record στο ιδιο bucket(overflowed) που ηταν ηδη αποθηκευμένο
      //απλα σε διαφορετικη θέση
      unsigned long old_offset = INT_SIZE;
      counter_old++;

      char* string_counter = itos(counter_old);
      memcpy(bucket +sizeof(char)*old_offset,string_counter,strlen(string_counter));
      free(string_counter);

      old_offset += INT_SIZE + sizeof(char)*RECORD_SIZE*(counter_old-1);

      memcpy(bucket + old_offset,string_id,strlen(string_id));

      old_offset += INT_SIZE;
      memcpy(bucket + old_offset,name,strlen(name));

      old_offset += NAME_SIZE;
      memcpy(bucket + old_offset,surname,strlen(surname));

      old_offset += SURNAME_SIZE;
      memcpy(bucket + old_offset,city,strlen(city));

    }
    //κανουμε καταλληλα free
    free(name);
    free(surname);
    free(city);
    free(string_id);

    //Εδω ελέγχεται η περίπτωση που τελείωσαν οι αποθηκευσεις των παλιων records 
    //και αποθεκευουμε το καινουργιο record
    //επισης ενημερώνουμε και το local depth του καινουργιου bucket
    if(rec == MAX_RECORDS - 1){
      memcpy(new_block_data,local_depth,strlen(local_depth));

      char* new_hash_value = hashFunction(record.id,atoi(local_depth));
      int new_pointer = get_bucket(new_hash_value,atoi(local_depth),dir);

      //υπαρχουν θεσεις και στα 2 buckets οποτε απλα καλουμε την store_record()
      BF_GetBlock(index,new_pointer,block);
      char* new_bucket = BF_Block_GetData(block);
      store_record(record,new_bucket);
      free(new_hash_value);
    }
  }
  free(local_depth);
  BF_Block_Destroy(&block);
}

//assign appropriate pointers after split
void pointers_adapt(int new_depth, char* dir, int overflowed_bucket, int last){
  int bit_offset = 0;
  int flag = 0;

  //διασχίζουμε όλο το directory και εναλλάξ αποφασίζουμε όταν βρισκουμε
  //pointer ενος hash block που δειχνει στο overflowed bucket
  //αμα θα αλλαξει και θα δειχνει στο καινουργιο που δημιουργήθηκε
  //από το split ή θα δειχνει στο ίδιο
  for(int i=0; i<pow(2,new_depth); i++){

    int key = get_int(bit_offset,INT_SIZE,dir);
    bit_offset += sizeof(char)*INT_SIZE;

    int pointer = get_int(bit_offset,INT_SIZE,dir);
    if(pointer == overflowed_bucket && flag==0){
      char* string_last = itos(last);
      memcpy(dir + bit_offset, string_last, strlen(string_last));
      free(string_last);
      flag++;
    }
    else if(pointer == overflowed_bucket){
      flag--;
    }
    bit_offset += sizeof(char)*INT_SIZE;
  }
}

void dirty_unpin_all(int indexDesc){
  BF_Block *block;
  BF_Block_Init(&block);

  int num_blocks;
  if(BF_GetBlockCounter(indexDesc,&num_blocks) != BF_OK)
    BF_PrintError(BF_GetBlockCounter(indexDesc,&num_blocks));
  
  for(int i=0; i<num_blocks; i++){
    BF_GetBlock(indexDesc,i,block);
    BF_Block_SetDirty(block);
    BF_UnpinBlock(block);
  }
  BF_Block_Destroy(&block);
}

typedef struct info{
  char file_name[20];
  int index;
}Info;

Info open_files[MAX_OPEN_FILES];      //οσα files ειναι ανοικτα atm
int file_create_counter;              //οσα files εχουν γινει create
int file_open_counter;                //οσα files εχουν γινει open

HT_ErrorCode HT_Init() {
  //αρχικοποιούμε τον πίνακα και τους counters
  file_open_counter = 0; 
  file_create_counter = 0;
  for(int i=0; i<MAX_OPEN_FILES; i++){
    open_files[i].index = -1;
  }
  return HT_OK;
}

HT_ErrorCode HT_CreateIndex(const char *filename, int depth) {

  //Δίνουμε οντότητα στην δομή
  BF_Block *block;
  BF_Block_Init(&block);

  int index;

  //Δημιουργούμε το file
  if(BF_CreateFile(filename) != BF_OK)
    return HT_ERROR;

  if(BF_OpenFile(filename, &index) != BF_OK)
    return HT_ERROR;

  //Δεσμέυουμε info block για το αρχείο
  if(BF_AllocateBlock(index, block) != BF_OK)
    return HT_ERROR;
  
  //Δεσμέυουμε directory block για το αρχείο
  for(int dir_block=0; dir_block<DIR_BLOCKS; dir_block++){
    if(BF_AllocateBlock(index, block) != BF_OK)
      return HT_ERROR;
  }

  //Επεξεργαζομαστε το info block
  BF_GetBlock(index, 0, block);
  char* info = BF_Block_GetData(block);
  char* depth_str = itos(depth);
  memcpy(info, depth_str, strlen(depth_str));
  free(depth_str);

  //Επεξεργασία και αρχικοποιηση του dir block
  BF_GetBlock(index, 1, block);
  char* dir = BF_Block_GetData(block);
  make_dir(depth,dir);

  //Δεσμέυουμε κατάλληλα buckets blocks για το αρχείο και τα αρχικοποιούμε (local + counter)
  for(int bucket=0; bucket< pow(2,depth); bucket++){
    if(BF_AllocateBlock(index, block) != BF_OK)
      return HT_ERROR;

    char* bucket_data = BF_Block_GetData(block);
    char* local_depth = itos(depth);
    memcpy(bucket_data, local_depth, strlen(local_depth));
    
    //Το 0 ειναι το counter των records
    memcpy(bucket_data + sizeof(char)*INT_SIZE, "0", strlen("0"));
    free(local_depth);

    if(bucket+1 >= BF_BUFFER_SIZE - (DIR_ENDS+1))
      break;
  }

  dirty_unpin_all(index);
  BF_CloseFile(index);
  file_create_counter++;
  BF_Block_Destroy(&block);
  return HT_OK;
}

HT_ErrorCode HT_OpenIndex(const char *fileName, int *indexDesc){

  if(BF_OpenFile(fileName, indexDesc) != BF_OK)
    return HT_ERROR;

  //αντιγράφουμε στο global πίνακά μας το filename και index από το αρχείο που ανοιγουμε
  strcpy(open_files[file_open_counter].file_name,fileName);
  open_files[file_open_counter].index = *indexDesc;
  //και ενημερώνουμε τα ανοικτά αρχεία
  file_open_counter++;
  return HT_OK;
}

HT_ErrorCode HT_CloseFile(int indexDesc) {

  int num_blocks;
  BF_GetBlockCounter(indexDesc, &num_blocks);

  BF_Block *block;
  BF_Block_Init(&block); 

  //κανουμε unpin όλα τα blocks
  for(int nblock=0; nblock < num_blocks; nblock++){
    BF_GetBlock(indexDesc, nblock, block);
    if(BF_UnpinBlock(block) != BF_OK)
      return HT_ERROR;
  }

  //και βρισκουμε το filename στον πινακά μας και το διαγράφουμε
  //και ενημερώνουμε το index
  for(int i=0; i<MAX_OPEN_FILES; i++){
    if(strcmp(open_files[i].file_name,"") != 0 && open_files[i].index == indexDesc){
      strcpy(open_files[i].file_name, "");
      open_files[i].index = -1;
    }
  }
  
  //το κανουμε close και μειώνουμε τον counter
  if(BF_CloseFile(indexDesc) != BF_OK){
    file_open_counter--;
    return HT_ERROR;
  }
  BF_Block_Destroy(&block);
  return HT_OK;
}

HT_ErrorCode HT_InsertEntry(int indexDesc, Record record) {

  BF_Block* block;
  BF_Block_Init(&block);

  //Παιρνουμε το global depth απο το info block
  BF_GetBlock(indexDesc,0,block);
  char* info  = BF_Block_GetData(block);
  int global_depth = get_int(0,INT_SIZE,info);

  //Παιρνουμε το dir
  BF_GetBlock(indexDesc,1,block);
  char* dir  = BF_Block_GetData(block);

  //Και παιρνουμε το bucket στο οποιο θα πρέπει να μπει το record
  int id = record.id;
  char* hash_value = hashFunction(id,global_depth);
  int pointer = get_bucket(hash_value,global_depth,dir);
  free(hash_value);

  BF_GetBlock(indexDesc, pointer, block);
  char* bucket = BF_Block_GetData(block);

  //Δοκιμάζουμε να αποθηκεύσουμε το record
  //Aν δεν τα καταφέρουμε υπάρχουν 2 περιπτώσεις
  if(store_record(record,bucket) == -1){

    //παιρνουμε το local depth
    int local_depth = get_int(0,INT_SIZE,bucket);

    //case 1
    if(local_depth == global_depth){
      //ενημερωση του global depth
      char* new_global_depth = itos(global_depth + 1);
      memcpy(info,new_global_depth,strlen(new_global_depth));

      //ενημερωση του local depth
      char* new_local_depth = itos(local_depth + 1);
      memcpy(bucket,new_local_depth,strlen(new_local_depth));

      //και πραγματοποιείται ο διπλασιασμός του dir με τις απαραίτητες αλλαγές
      expand_dict(global_depth+1,dir,pointer,get_last_bucket(indexDesc));
      split(indexDesc,bucket,record,dir);

      free(new_global_depth);
      free(new_local_depth);
    }
    //case 2
    else{
      //ενημερωση του local depth
      char* new_local_depth = itos(local_depth + 1);
      memcpy(bucket,new_local_depth,strlen(new_local_depth));

      //και πραγματοποιείται η καταλληλη ενημέρωση του dir
      pointers_adapt(global_depth+1,dir,pointer,get_last_bucket(indexDesc));
      split(indexDesc,bucket,record,dir);

      free(new_local_depth);
    }
  }
  dirty_unpin_all(indexDesc);
  BF_Block_Destroy(&block);
  return HT_OK;
}

HT_ErrorCode HT_PrintAllEntries(int indexDesc, int *id) {
  BF_Block *block;
  BF_Block_Init(&block);

  int num_blocks;
  BF_GetBlockCounter(indexDesc,&num_blocks);

  //αμα δεν υπάρχει id τοτε εκτυπώνουμε όλες τις εγραφές των buckets
  if(id == NULL){
    //Για κάθε bucket εκτυπώνουμε τα περιεχόμενά του
    for(int i=DIR_BLOCKS+1; i<num_blocks; i++){
      BF_GetBlock(indexDesc, i, block);
      char* data = BF_Block_GetData(block);

      printf("========== Bucket %d ===========\n",i-(DIR_BLOCKS+1));
      print_block(data);
      printf("================================\n\n");
    }
  }
  //αλλιως θα ψάξουμε τα records με το συγκεκριμένο id
  else{
    printf("We want the record with id %d\n",*id);
    BF_GetBlock(indexDesc,0,block);
    char* info = BF_Block_GetData(block);

    int global_depth = get_int(0,INT_SIZE,info);
    char* hash_value = hashFunction(*id,global_depth);

    //παιρνουμε το directory
    BF_GetBlock(indexDesc,1,block);
    char* dir = BF_Block_GetData(block);

    //και το bucket
    int pointer = get_bucket(hash_value,global_depth,dir);
    free(hash_value);

    BF_GetBlock(indexDesc,pointer,block);
    char* data = BF_Block_GetData(block);

    unsigned long offset = 0;
    int count = get_int(INT_SIZE,INT_SIZE,data);    //το count είναι τα πόσα records υπάρχουν στο bucket
    int flag = 0;
    for(int i=0; i<count; i++){

      offset = sizeof(char)*INT_SIZE*2 + sizeof(char)*RECORD_SIZE*i;
      int temp_id = get_int(offset,INT_SIZE,data);

      //αν βρούμε το id τοτε εκτυπώνουμε τα δεδομένα του record
      if(temp_id == *id){
        flag = 1;
        offset += sizeof(char)*INT_SIZE;
        char* name = get_string(offset,NAME_SIZE,data);
        offset += sizeof(char)*NAME_SIZE;
        char* surname = get_string(offset,NAME_SIZE,data);
        offset += sizeof(char)*SURNAME_SIZE;
        char* city = get_string(offset,NAME_SIZE,data);
        printf("Id %d\nName: %s \nSurname: %s \nCity: %s\n\n",temp_id,name,surname,city);
        free(name);
        free(surname);
        free(city);
      }
    }
    if(flag == 0)
      printf("There is no record with id:%d\n",*id);
  }
  BF_Block_Destroy(&block);
  return HT_OK;
}

HT_ErrorCode HashStatistics(const char *filename){
  BF_Block *block;
  BF_Block_Init(&block);

  //Βρίσκουμε το file και διατρέχουμε τα buckets του βλέπωντας το counter τους
  for(int i=0; i<file_open_counter; i++){
    if(strcmp(open_files[i].file_name,filename) == 0){
      int index = open_files[i].index;
      int num_blocks;
      BF_GetBlockCounter(index,&num_blocks);
      printf("The file has %d blocks\n",num_blocks);

      if(num_blocks == DIR_BLOCKS + 1){
        printf("No buckets in the file\n");
        return HT_ERROR;
      }

      int min = MAX_RECORDS + 1;
      int max = -1;
      int sum = 0;

      for(int j=DIR_BLOCKS+1; j<num_blocks; j++){
        BF_GetBlock(index,j,block);
        char* data = BF_Block_GetData(block);
        unsigned long offset = 0;
        int count = get_int(INT_SIZE,INT_SIZE,data);
        if(count < min)
          min = count;
        if(count > max)
          max = count;
        sum += count;
      }
      sum = sum/(num_blocks-2);
      printf("Min:%d\nMax:%d\nAvarage:%d\n",min,max,sum);
    }
  }
  BF_Block_Destroy(&block);
  return HT_OK;
}