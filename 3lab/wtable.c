#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "wtable.h"

unsigned int hash1(int key){ //HashFAQ6
	char* str = (char*)&key;

    unsigned int hash = 0;

    for (int i = 0; i < sizeof(int); i++)
    {
        hash += (unsigned char)(*str);
        hash += (hash << 10);
        hash ^= (hash >> 6);
        str++;
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    return hash;
}

unsigned int hash2_in(int key){ //HashRot13
	char* str = (char*)&key;

    unsigned int hash = 0;

    for(int i = 0; i < sizeof(int); i++)
    {
        hash += (unsigned char)(*str);
        hash -= (hash << 13) | (hash >> 19);
        str++;
    }

    return hash;

}

unsigned int hash2(int key, int msize){

	unsigned int hash = hash2_in(key);
	
	while(!(hash % msize)){
		hash = hash2_in(hash);
	}

    return hash % msize;

}

void create_wtable(int msize){
	Table table;
	table.csize = 0;
	table.msize = msize;

	FILE* file = fopen("table.bin", "wb");
    fwrite(&table, sizeof(Table), 1, file);

    KeySpace1* ks1 = (KeySpace1*)calloc(1, sizeof(KeySpace1));
    for(int i = 0; i < msize; i++){
    	fwrite(ks1, sizeof(KeySpace1), 1, file);
	}

    KeySpace2* ks2 = (KeySpace2*)calloc(1, sizeof(KeySpace2));
    for(int i = 0; i < msize; i++){
    	fwrite(ks2, sizeof(KeySpace2), 1, file);
	}

    fclose(file);
    free(ks1);
    free(ks2);
}

void add_key1(Table table, KeySpace1* ks1, int j, const char* key1, int rel, FILE* file){
	memmove(&(ks1[j+1]), &(ks1[j]), (table.csize - j) * sizeof(KeySpace1));
	Item item;
	
	for(int i = 0; i < table.csize - j; i++){
		rewind(file);
    	fseek(file, ks1[j+i+1].pos, SEEK_CUR);
    	fread(&item, sizeof(Item), 1, file);
    	item.ind1++;
    	fseek(file, -sizeof(Item), SEEK_CUR);
    	fwrite(&item, sizeof(Item), 1, file);
    }
	memcpy(ks1[j].key, key1, N);
	ks1[j].release = rel;
}

void read_table(Table* table, KeySpace1** ks1, KeySpace2** ks2, FILE* file){
    fread(table, sizeof(Table), 1, file);

    *ks1 = (KeySpace1*)malloc(table->msize * sizeof(KeySpace1));
	*ks2 = (KeySpace2*)malloc(table->msize * sizeof(KeySpace2));
    fread(*ks1, sizeof(KeySpace1), table->msize, file);
    fread(*ks2, sizeof(KeySpace2), table->msize, file);
}

void free_RAM(KeySpace1* ks1, KeySpace2* ks2, FILE* file){
	free(ks1);
    free(ks2);
    fclose(file);
}

int insert_wtable(char key1[N], int key2, int x, int y, char* string){
	Table table;
	KeySpace1* ks1;
	KeySpace2* ks2;

	FILE* file = fopen("table.bin", "r+b");

	read_table(&table, &ks1, &ks2, file);

    if(table.csize == table.msize){
    	free_RAM(ks1, ks2, file);
    	return -1;
    }

	if(table.csize){
		for(int i = 0; i < table.msize; i++){
			if(ks2[i].busy && key2 == ks2[i].key){
				free_RAM(ks1, ks2, file);
				return -1; //check 2-ond place
			}
		}
	}
	int ind1 = 0, ind2 = 0;

	//insert into the first table
	if(table.csize){	
		int left = 0, right = table.csize - 1;
		int j, status = 1, rel = 0;
		while(left <= right && status){
			j = (left + right) / 2;
			status = strcmp(ks1[j].key, key1);
			if(status < 0)
				left = j + 1;
			else
				right = j - 1;
		}
		if(status < 0){
			add_key1(table, ks1, j + 1, key1, 0, file);
			ind1 = j + 1;
		}else if(status > 0){
			add_key1(table, ks1, j, key1, 0, file);
			ind1 = j;
		}else{
			while(!strcmp(ks1[j].key, key1)){
				if(!(j + 1 < table.csize)){
					rel = ks1[j].release;
					j++;
					break;
				}
				rel = ks1[j++].release;
			}
			add_key1(table, ks1, j, key1, rel + 1, file);
			ind1 = j;
		}
	}else{
		strcpy(ks1[0].key, key1);
		ks1[0].release = 0;
		ind1 = 0;
	}

	//insert into the second table
	for(int i = 0; i < table.msize; i++){
		int index = (hash1(key2) + i * hash2(key2, table.msize)) % table.msize;
		if(!(ks2[index].busy)){
			ks2[index].busy = 1;
			ks2[index].key = key2;
			ind2 = index;
			break;
		}
	}
	/*
	fclose(file);

	file = fopen("table.bin", "ab");*/
	fseek(file, 0, SEEK_END);/**/

	Item item;
	item.ind1 = ind1;
	item.ind2 = ind2;
	item.del = 0;
	item.info.x = x;
	item.info.y = y;
	item.info.len = strlen(string);
	fwrite(&item, sizeof(Item), 1, file);
	long int pos = ftell(file) - sizeof(Item);
	fwrite(string, sizeof(char), strlen(string), file);
	fclose(file);
	
	ks1[ind1].pos = pos;
	ks2[ind2].pos = pos;
	table.csize++;
	

	file = fopen("table.bin", "r+b");
	fwrite(&table, sizeof(Table), 1, file);
	fwrite(ks1, sizeof(KeySpace1), table.msize, file);
    fwrite(ks2, sizeof(KeySpace2), table.msize, file);
	free_RAM(ks1, ks2, file);
}

int bin_search(Table table, KeySpace1* ks1, char* key1){
	int left = 0, right = table.csize - 1;
	int ind1, status = 1;
	while(left <= right && status){
		ind1 = (left + right) / 2;
		status = strcmp(ks1[ind1].key, key1);
		if(status < 0)
			left = ind1 + 1;
		else
			right = ind1 - 1;
	}
	if(status != 0) return -1;
	for(;ind1 >= 0 && !strcmp(ks1[ind1].key, key1); ind1--){}
	ind1++;
	return ind1;
}

Item read_item1(FILE* file, KeySpace1* ks1, int ind1){
	Item item;
	rewind(file);
	fseek(file, ks1[ind1].pos, SEEK_CUR);
	fread(&item, sizeof(Item), 1, file);
	return item;
}

Item read_item2(FILE* file, KeySpace2* ks2, int ind2){
	Item item;
	rewind(file);
	fseek(file, ks2[ind2].pos, SEEK_CUR);
	fread(&item, sizeof(Item), 1, file);
	return item;
}

InfoS* read_infos(Item item, FILE* file){
	InfoS* infos = (InfoS*)malloc(sizeof(InfoS));
	infos->info.x = item.info.x;
	infos->info.y = item.info.y;
	infos->info.len = item.info.len;
	infos->string = (char*)malloc(item.info.len + 1);
	fread(infos->string, sizeof(char), item.info.len, file);
	infos->string[item.info.len] = '\0';
	return infos;
}

InfoSR* read_infosr(Table table, KeySpace1* ks1, int ind1, int ind0, char key1[N], int count, FILE* file){
	InfoSR* information = (InfoSR*)calloc(1,(count + 1) * sizeof(InfoSR));
	for(;ind1 < table.csize && !strcmp(ks1[ind1].key, key1); ind1++){
		Item item = read_item1(file, ks1, ind1);
		information[ind1 - ind0].info.x = item.info.x;
		information[ind1 - ind0].info.y = item.info.y;
		information[ind1 - ind0].string = (char*)malloc(item.info.len + 1);
		fread(information[ind1 - ind0].string, sizeof(char), item.info.len, file);
		information[ind1 - ind0].string[item.info.len] = '\0';
		information[ind1 - ind0].release = ks1[ind1].release;
	}
	information[ind1 - ind0].string = NULL;
	return information;
}

InfoS* KS1_1_search_wtable(char key1[N], int key2){ //bin search  and delete
	Table table;
	KeySpace1* ks1;
	KeySpace2* ks2;

	FILE* file = fopen("table.bin", "rb");

	read_table(&table, &ks1, &ks2, file);

    if(!table.csize){
		free_RAM(ks1, ks2, file);
		return NULL;
	}

	int ind1, ind2;

	ind1 = bin_search(table, ks1, key1);

	if(ind1 == -1){
		free_RAM(ks1, ks2, file);
		return NULL;
	}

	for(;ind1 < table.csize && !strcmp(ks1[ind1].key, key1); ind1++){
		Item item = read_item1(file, ks1, ind1);
		ind2 = item.ind2;
		if(ks2[ind2].key == key2 && !item.del){
			InfoS* infos = read_infos(item, file);
			free_RAM(ks1, ks2, file);
			return infos;
		}
	}

	free_RAM(ks1, ks2, file);

	return NULL;
}

void KS1_1_delete_wtable(char key1[N], int key2){ //bin search  and delete
	Table table;
	KeySpace1* ks1;
	KeySpace2* ks2;

	FILE* file = fopen("table.bin", "r+b");

	read_table(&table, &ks1, &ks2, file);

    if(!table.csize){
		free_RAM(ks1, ks2, file);
		return;
	}

	int ind1, ind2;

	ind1 = bin_search(table, ks1, key1);

	if(ind1 == -1){
		free_RAM(ks1, ks2, file);
		return;
	}

	for(;ind1 < table.csize && !strcmp(ks1[ind1].key, key1); ind1++){
		Item item = read_item1(file, ks1, ind1);
		ind2 = item.ind2;
		if(ks2[ind2].key == key2 && !item.del){
			item.del = 1;
			fseek(file, -sizeof(Item), SEEK_CUR);
			fwrite(&item, sizeof(Item), 1, file);			
		}
	}

	free_RAM(ks1, ks2, file);
}

InfoS* KS2_1_search_wtable(char* key1, int key2){ //hash search and delete
	Table table;
	KeySpace1* ks1;
	KeySpace2* ks2;

	FILE* file = fopen("table.bin", "rb");

	read_table(&table, &ks1, &ks2, file);

	if(!table.csize){
		free_RAM(ks1, ks2, file);
		return NULL;
	}

	int ind1, ind2;

	for(int i = 0; i < table.msize; i++){
		ind2 = (hash1(key2) + i * hash2(key2, table.msize)) % table.msize;
		if(ks2[ind2].busy && ks2[ind2].key == key2){
			Item item = read_item2(file, ks2, ind2);
			ind1 = item.ind1;
			if(!strcmp(ks1[ind1].key, key1)){
				InfoS* infos = read_infos(item, file);
				free_RAM(ks1, ks2, file);
				return infos;
			}
		}
	}
	free_RAM(ks1, ks2, file);
	return NULL;
}

void KS2_1_delete_wtable(char* key1, int key2){ //hash search and delete
	Table table;
	KeySpace1* ks1;
	KeySpace2* ks2;

	FILE* file = fopen("table.bin", "r+b");

	read_table(&table, &ks1, &ks2, file);

	if(!table.csize){
		free_RAM(ks1, ks2, file);
		return;
	}

	int ind1, ind2;

	for(int i = 0; i < table.msize; i++){
		ind2 = (hash1(key2) + i * hash2(key2, table.msize)) % table.msize;
		if(ks2[ind2].busy && ks2[ind2].key == key2){
			Item item = read_item2(file, ks2, ind2);
			ind1 = item.ind1;
			if(!strcmp(ks1[ind1].key, key1)){
				item.del = 1;
				fseek(file, -sizeof(Item), SEEK_CUR);
				fwrite(&item, sizeof(Item), 1, file);
			}
		}
	}
	free_RAM(ks1, ks2, file);
}


InfoSR* KS1_2_search_wtable(char key1[N]){ //bin search  and delete
	Table table;
	KeySpace1* ks1;
	KeySpace2* ks2;

	FILE* file = fopen("table.bin", "rb");

	read_table(&table, &ks1, &ks2, file);

    if(!table.csize){
		free_RAM(ks1, ks2, file);
		return NULL;
	}

	int ind1, ind2, ind0;
	ind1 = bin_search(table, ks1, key1);
	if(ind1 == -1){
		free_RAM(ks1, ks2, file);
		return NULL;
	}
	ind0 = ind1;
	int count = 0;
	for(;ind1 < table.csize && !strcmp(ks1[ind1].key, key1); ind1++){
		count+=1;
	}
	ind1 = ind0;

	InfoSR* information = read_infosr(table, ks1, ind1, ind0, key1, count, file);
	
	free_RAM(ks1, ks2, file);
	return information;
}

void KS1_2_delete_wtable(char key1[N]){ //bin search  and delete
	Table table;
	KeySpace1* ks1;
	KeySpace2* ks2;

	FILE* file = fopen("table.bin", "r+b");

	read_table(&table, &ks1, &ks2, file);

    if(!table.csize){
		free_RAM(ks1, ks2, file);
		return;
	}

	int ind1, ind2, ind0;
	ind1 = bin_search(table, ks1, key1);
	if(ind1 == -1){
		free_RAM(ks1, ks2, file);
		return;
	}
	ind0 = ind1;
	int count = 0;
	for(;ind1 < table.csize && !strcmp(ks1[ind1].key, key1); ind1++){
		count+=1;
	}
	ind1 = ind0;

	for(;ind1 < table.csize && !strcmp(ks1[ind1].key, key1); ind1++){
		Item item = read_item1(file, ks1, ind1);
		item.del = 1;
		fseek(file, -sizeof(Item), SEEK_CUR);
		fwrite(&item, sizeof(Item), 1, file);
	}
	
	free_RAM(ks1, ks2, file);
}

InfoS* KS2_2_search_wtable(int key2){ //hash search and delete
	Table table;
	KeySpace1* ks1;
	KeySpace2* ks2;

	FILE* file = fopen("table.bin", "rb");

	read_table(&table, &ks1, &ks2, file);

	if(!table.csize){
		free_RAM(ks1, ks2, file);
		return NULL;
	}

	int ind1, ind2;

	for(int i = 0; i < table.msize; i++){
		ind2 = (hash1(key2) + i * hash2(key2, table.msize)) % table.msize;
		if(ks2[ind2].busy && ks2[ind2].key == key2){
			Item item = read_item2(file, ks2, ind2);
			InfoS* infos = read_infos(item, file);
			free_RAM(ks1, ks2, file);
			return infos;
		}
	}
	free_RAM(ks1, ks2, file);
	return NULL;
}

void KS2_2_delete_wtable(int key2){ //hash search and delete
	Table table;
	KeySpace1* ks1;
	KeySpace2* ks2;

	FILE* file = fopen("table.bin", "r+b");

	read_table(&table, &ks1, &ks2, file);

	if(!table.csize){
		free_RAM(ks1, ks2, file);
		return;
	}

	int ind1, ind2;

	for(int i = 0; i < table.msize; i++){
		ind2 = (hash1(key2) + i * hash2(key2, table.msize)) % table.msize;
		if(ks2[ind2].busy && ks2[ind2].key == key2){
			Item item = read_item2(file, ks2, ind2);
			item.del = 1;
			fseek(file, -sizeof(Item), SEEK_CUR);
			fwrite(&item, sizeof(Item), 1, file);
		}
	}
	free_RAM(ks1, ks2, file);
}

InfoS* search_releases_wtable(char key1[N], int rel){ //bin search  and delete
	Table table;
	KeySpace1* ks1;
	KeySpace2* ks2;

	FILE* file = fopen("table.bin", "rb");

	read_table(&table, &ks1, &ks2, file);

    if(!table.csize){
		free_RAM(ks1, ks2, file);
		return NULL;
	}

	int ind1;

	ind1 = bin_search(table, ks1, key1);

	if(ind1 == -1){
		free_RAM(ks1, ks2, file);
		return NULL;
	}

	for(;ind1 < table.csize && !strcmp(ks1[ind1].key, key1); ind1++){
		Item item = read_item1(file, ks1, ind1);
		if(ks1[ind1].release == rel && !item.del){
			InfoS* infos = read_infos(item, file);
			free_RAM(ks1, ks2, file);
			return infos;
		}
	}

	free_RAM(ks1, ks2, file);

	return NULL;
}

void delete_releases_wtable(char key1[N], int rel){ //bin search  and delete
	Table table;
	KeySpace1* ks1;
	KeySpace2* ks2;

	FILE* file = fopen("table.bin", "r+b");

	read_table(&table, &ks1, &ks2, file);

    if(!table.csize){
		free_RAM(ks1, ks2, file);
		return;
	}

	int ind1;

	ind1 = bin_search(table, ks1, key1);

	if(ind1 == -1){
		free_RAM(ks1, ks2, file);
		return;
	}

	for(;ind1 < table.csize && !strcmp(ks1[ind1].key, key1); ind1++){
		Item item = read_item1(file, ks1, ind1);
		if(ks1[ind1].release == rel && !item.del){
			item.del = 1;
			fseek(file, -sizeof(Item), SEEK_CUR);
			fwrite(&item, sizeof(Item), 1, file);			
		}
	}

	free_RAM(ks1, ks2, file);
}

void show_wtable(){
	Table table;
	KeySpace1* ks1;
	KeySpace2* ks2;

	FILE* file = fopen("table.bin", "rb");

	read_table(&table, &ks1, &ks2, file);

    Item item;
    char* string;
    for(int i = 0; i < table.csize; i++){
    	rewind(file);
    	fseek(file, ks1[i].pos, SEEK_CUR);
    	fread(&item, sizeof(Item), 1, file);
    	if(item.del) continue;
    	string = (char*)malloc(item.info.len + 1);
    	fread(string, sizeof(char), item.info.len, file);
    	string[item.info.len] = '\0';
    	printf("x: %d y: %d s: %s key1: %s rel: %d key2: %d\n", item.info.x, item.info.y, string, ks1[item.ind1].key, ks1[item.ind1].release, ks2[item.ind2].key);
    	free(string);
    }
    //system("hexdump -C table.bin");
    free_RAM(ks1, ks2, file);
}

void clear_table(){
	FILE* file = fopen("table.bin", "wb");
	fclose(file);
}

void garbage_collector(){
	Table table;
	KeySpace1* ks1;
	KeySpace2* ks2;

	FILE* file = fopen("table.bin", "r+b");

	read_table(&table, &ks1, &ks2, file);

	long int start = ftell(file);
	fseek(file, 0, SEEK_END);
	long int end = ftell(file);

	char* payload = (char*)malloc(end - start);
	fseek(file, start - end, SEEK_CUR);
	fread(payload, sizeof(char), end - start, file);

	Item item;
	memcpy(&item, payload, sizeof(Item));
	printf("%d\n", item.info.x);

	free(payload);
	free_RAM(ks1, ks2, file);
}

int main(int argc, char const *argv[])
{
	create_wtable(57);

	insert_wtable("qqqq", 1,2,3, "1wrrr");

	insert_wtable("pppp", 4,5,6, "2wrrr");

	insert_wtable("pppp", 7,5,6, "2wrrr");

	insert_wtable("pppppppppp", 5,7,8, "3wrrr");

	insert_wtable("qqqq", 2,9,10, "4wrrr");

	insert_wtable("pppp", 6,11,12, "5wrrr");
	InfoS* infos = KS2_2_search_wtable(6);
	if(infos){ 
		printf("%d %d %s\n", infos->info.x, infos->info.y, infos->string );
		free(infos->string);
		free(infos);
	}
	//KS2_1_delete_wtable("qqqq", 1);

	InfoSR* infosr = KS1_2_search_wtable("pppp");
	if(infosr){
		for(int i = 0 ;infosr[i].string; i++){
			printf("%d %d %d %s\n", infosr[i].release, infosr[i].info.x, infosr[i].info.y, infosr[i].string);
			free(infosr[i].string);
		}
	}
	free(infosr);

	infos = search_releases_wtable("pppp", 0);
	if(infos){ 
		printf("%d %d %s\n", infos->info.x, infos->info.y, infos->string );
		free(infos->string);
		free(infos);
	}



	show_wtable();

	garbage_collector();


	return 0;
}