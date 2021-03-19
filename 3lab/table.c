#include "table.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void bin_search(char* s){
	int left = 0;
	int right = strlen(s) - 1;
	int j = 0;
	int key_index = - 1;
	while(left <= right && !(s[j] == 'n')){
		j = (left + right) / 2;

		if(s[j] < 'n')
			left = j + 1;
		else
			right = j - 1;
	}
	if(s[j] < 'n'){
		memcpy(s + (j+1)*sizeof(char), s + j*sizeof(char), 30 - j);
		s[j+1] = 'n';
	}else if(s[j] > 'n'){
		memcpy(s + (j)*sizeof(char), s + (j-1)*sizeof(char), 30 - 1 - j);
		s[j] = 'n';
	}
	else{
		while(s[j] == 'n')
			j++;
		memcpy(s + (j)*sizeof(char), s + (j-1)*sizeof(char), 30 - 1 - j);
		s[j] = 'N';
	}
	printf("%s\n", s);

}

unsigned int hash1(int key){ //HashFAQ6
	char* str = (char*)&key;

    unsigned int hash = 0;

    for (; *str; str++)
    {
        hash += (unsigned char)(*str);
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    return hash ? hash : hash1(1);
}

unsigned int hash2(int key){ //HashRot13
	char* str = (char*)&key;

    unsigned int hash = 0;

    for(; *str; str++)
    {
        hash += (unsigned char)(*str);
        hash -= (hash << 13) | (hash >> 19);
    }

    return hash ? hash : hash2(1);

}

char* add_key1(Table* table, int j,const char* key1, int rel){
	memmove(&(table->ks1[j+1]), &(table->ks1[j]), (table->csize - j) * sizeof(KeySpace1));
	for(int i = 0; i < table->csize - j; i++)
		table->ks1[j+i+1].item->ind1 += 1;
	table->ks1[j].key = (char*)malloc(strlen(key1));
	strcpy(table->ks1[j].key, key1);
	table->ks1[j].release = rel;
}

//----------------------------------------------------------------------------------------

Table* init_table(){
	return calloc(1, sizeof(Table));
}

void create_table(Table* table, int msize){
	table->msize = msize;
	table->csize = 0;
	table->ks1 = (KeySpace1*)malloc(msize * sizeof(KeySpace1));
	table->ks2 = (KeySpace2*)calloc(1, msize * sizeof(KeySpace2));
}

int insert_table(Table* table,const char* key1, int key2, Info info){
	if(table->csize == table->msize) return -1;
	if(table->csize){
		for(int i = 0; i < table->msize; i++){
			if(table->ks2[i].busy && key2 == table->ks2[i].key) return -1; //check 2-ond place
		}
	}
	int ind1, ind2;

	if(table->csize){	
		int left = 0, right = table->csize - 1;
		int j, status = 1, rel;
		while(left <= right && status){
			j = (left + right) / 2;
			status = strcmp(table->ks1[j].key, key1);
			if(status < 0)
				left = j + 1;
			else
				right = j - 1;
		}
		if(status < 0){
			add_key1(table, j + 1, key1, 0);
			ind1 = j + 1;
		}else if(status > 0){
			add_key1(table, j, key1, 0);
			ind1 = j;
		}else{
			while(!strcmp(table->ks1[j].key, key1)){
				if(!(j+1<table->csize)){
					j++;
					break;
				}
				rel = table->ks1[j++].release;
			}
			add_key1(table, j, key1, rel+1);
			ind1 = j;
		}
	}else{
		table->ks1[0].key = (char*)malloc(strlen(key1));
		strcpy(table->ks1[0].key, key1);
		table->ks1[0].release = 0;
		ind1 = 0;
	}

	for(int i = 0; i < table->msize; i++){
		int index = (hash1(key2) + i * (hash2(key2) % table->msize)) % table->msize;
		if(!(table->ks2[index].busy)){
			table->ks2[index].busy = 1;
			table->ks2[index].key = key2;
			ind2 = index;
			break;
		}
	}

	table->ks1[ind1].item = (Item*)malloc(sizeof(Item));
	memcpy(&(table->ks1[ind1].item->info), &info, sizeof(info));
	table->ks1[ind1].item->info.string = (char*)malloc(strlen(info.string));
	strcpy(table->ks1[ind1].item->info.string, info.string);
	table->ks1[ind1].item->ind1 = ind1;
	table->ks1[ind1].item->ind2 = ind2;
	table->ks2[ind2].item = table->ks1[ind1].item;

	table->csize++;
}

//-----------------------------TWO KEYS---------------------------------------------------------

const Info* search1_table(Table* table, char* key1, int key2, int del){ //bin search  and delete
	if(!table->csize) return NULL;
	int left = 0, right = table->csize - 1;
	int ind1, ind2, status = 1;
	while(left <= right && status){
		ind1 = (left + right) / 2;
		status = strcmp(table->ks1[ind1].key, key1);
		if(status < 0)
			left = ind1 + 1;
		else
			right = ind1 - 1;
	}
	if(status != 0) return NULL;
	ind1 -= table->ks1[ind1].release;
	if(del){
		for(;ind1 < table->csize && !strcmp(table->ks1[ind1].key, key1); ind1++){
			ind2 = table->ks1[ind1].item->ind2;
			if(table->ks2[ind2].key == key2){
				table->ks2[ind2].busy = 0;
				free(table->ks1[ind1].item->info.string);
				free(table->ks1[ind1].item);
				free(table->ks1[ind1].key);
				memmove(&(table->ks1[ind1]), &(table->ks1[ind1+1]), (table->csize - ind1 - 1) * sizeof(KeySpace1));
				for(int i = ind1; i < table->csize - ind1 - 1; i ++){
					table->ks1[i].item->ind1--;
				}
				table->csize--;
				return NULL;
			}
		}
	}else{
		for(;ind1 < table->csize && !strcmp(table->ks1[ind1].key, key1); ind1++){
			ind2 = table->ks1[ind1].item->ind2;
			if(table->ks2[ind2].key == key2){
				return (const Info*)&(table->ks1[ind1].item->info);
			}
		}
	}
	return NULL;
}

const Info* search2_table(Table* table, char* key1, int key2, int del){ //hash_search and delete
	if(!table->csize) return NULL;
	int ind1, ind2;
	if(del){ //delete
		for(int i = 0; i < table->msize; i++){
			ind2 = (hash1(key2) + i * (hash2(key2) % table->msize)) % table->msize;
			if(table->ks2[ind2].busy && table->ks2[ind2].key == key2){
				ind1 = table->ks2[ind2].item->ind1;
				if(!strcmp(table->ks1[ind1].key, key1))
					table->ks2[ind2].busy = 0;
					free(table->ks1[ind1].item->info.string);
					free(table->ks1[ind1].item);
					free(table->ks1[ind1].key);
					memmove(&(table->ks1[ind1]), &(table->ks1[ind1+1]), (table->csize - ind1 - 1) * sizeof(KeySpace1));
					for(int i = ind1; i < table->csize - ind1 - 1; i ++){
						table->ks1[i].item->ind1--;
					}
					table->csize--;
					return NULL;
			}
		}
	}else{ //search
		for(int i = 0; i < table->msize; i++){
			ind2 = (hash1(key2) + i * (hash2(key2) % table->msize)) % table->msize;
			if(table->ks2[ind2].busy && table->ks2[ind2].key == key2){
				ind1 = table->ks2[ind2].item->ind1;
				if(!strcmp(table->ks1[ind1].key, key1))
						return (const Info*)&(table->ks2[ind2].item->info);
			}
		}
	}
	return NULL;
}

//-----------------------------ONE KEY---------------------------------------------------------

InfoR* search_elements1_table(Table* table, char* key1, int del){ //return massive with structures and null-term structure
	if(!table->csize) return NULL;
	int left = 0, right = table->csize - 1;
	int ind1, ind2, ind0, status = 1;
	while(left <= right && status){
		ind1 = (left + right) / 2;
		status = strcmp(table->ks1[ind1].key, key1);
		if(status < 0)
			left = ind1 + 1;
		else
			right = ind1 - 1;
	}
	if(status != 0) return NULL;
	ind1 -= table->ks1[ind1].release;
	ind0 = ind1;
	if(del){
		for(;ind1 < table->csize && !strcmp(table->ks1[ind1].key, key1); ind1++){
			ind2 = table->ks1[ind1].item->ind2;
			table->ks2[ind2].busy = 0;
			free(table->ks1[ind1].item->info.string);
			free(table->ks1[ind1].item);
			free(table->ks1[ind1].key);
		}
		memmove(&(table->ks1[ind0]), &(table->ks1[ind1]), (table->csize - ind1) * sizeof(KeySpace1));
		for(int i = ind0; i < ind1; i++){
			table->ks1[i].item->ind1 -= ind1 - ind0;
		}
		table->csize -= ind1 - ind0;
	}else{
		int count = 0;
		for(;ind1 < table->csize && !strcmp(table->ks1[ind1].key, key1); ind1++){
			count+=1;
		}
		ind1 = ind0;
		InfoR* information = (InfoR*)malloc((count + 1) * sizeof(InfoR));
		for(;ind1 < table->csize && !strcmp(table->ks1[ind1].key, key1); ind1++){
			memcpy(&(information[ind1 - ind0].info), &(table->ks1[ind1].item->info), sizeof(Info));
			information[ind1 - ind0].info.string = (char*)malloc(strlen(table->ks1[ind1].item->info.string));
			strcpy(information[ind1 - ind0].info.string, table->ks1[ind1].item->info.string);
			information[ind1 - ind0].release = table->ks1[ind1].release;
		}
		information[ind1 - ind0].info.string = NULL;
		return information;
	}
	return NULL;
}

Info* search_elements2_table(Table* table, int key2, int del){ // return one structure
	if(!table->csize) return NULL;
	int ind1, ind2;
	if(del){ //delete
		for(int i = 0; i < table->msize; i++){
			ind2 = (hash1(key2) + i * (hash2(key2) % table->msize)) % table->msize;
			if(table->ks2[ind2].busy && table->ks2[ind2].key == key2){
				ind1 = table->ks2[ind2].item->ind1;
				table->ks2[ind2].busy = 0;
				free(table->ks1[ind1].item->info.string);
				free(table->ks1[ind1].item);
				free(table->ks1[ind1].key);
				memmove(&(table->ks1[ind1]), &(table->ks1[ind1+1]), (table->csize - ind1) * sizeof(KeySpace1));
				for(int i = ind1; i < table->csize - 1; i ++){
					table->ks1[i].item->ind1--;
				}
				table->csize--;
				return NULL;
			}
		}
	}else{ //search
		for(int i = 0; i < table->msize; i++){
			ind2 = (hash1(key2) + i * (hash2(key2) % table->msize)) % table->msize;
			if(table->ks2[ind2].busy && table->ks2[ind2].key == key2){
				Info* info = (Info*)malloc(sizeof(Info));
				memcpy(info, &(table->ks2[ind2].item->info), sizeof(Info));
				info->string = (char*)malloc(strlen(table->ks2[ind2].item->info.string));
				strcpy(info->string, table->ks2[ind2].item->info.string);
				return info;
			}
		}
	}
	return NULL;
}

//-----------------------------SPECIAL SEARCH FOR A FIRST KEY SPACE---------------------------------------------------------

void search_all_releases(Table* table, char* key1){
	return;
}

void print_table(Table* table){
	for(int i = 0; i < table->csize; i++){
		printf("%d ", i);
		printf("Info: %d %d %s;\n", table->ks1[i].item->info.x, table->ks1[i].item->info.y, table->ks1[i].item->info.string);
		printf("  key1: %s,", table->ks1[i].key);
		printf("  rel %d;\n", table->ks1[i].release);
		int ind1 = table->ks1[i].item->ind1;
		int ind2 = table->ks1[i].item->ind2;
		printf("  key2: %d;\n", table->ks2[ind2].key);
		
	}
}

//----------------------------------------------------------------------------------------

int main(int argc, char const *argv[])
{
	Table* table = init_table();
	create_table(table, 31);
	char* s = (char*)malloc(30);
	for(int i = 0; i < 5; i++)
		s[i] = 'a';

	Info info1 = {3, 4, "qqweuweihu"};
	Info info2 = {5, 6, "jgfghjsbhj"};
	Info info3 = {7, 8, "jshhueghiuhkukj"};
	insert_table(table, "a", 1, info1);
	insert_table(table, "x", 2, info2);
	insert_table(table, "n", 3, info2);
	insert_table(table, "a", 4, info3);
	insert_table(table, "a", 5, info2);
	insert_table(table, "y", 6, info2);
	insert_table(table, "n", 7, info3);
	insert_table(table, "y", 8, info3);

	const Info* inf = search1_table(table, "y", 8, 0);
	if(inf && inf != (const Info*)table){
		printf("%d %d %s\n", inf->x, inf->y, inf->string);
	}

	const Info* inf1 = search2_table(table, "a", 2, 0);
	if(inf1 && inf1 != (const Info*)table){
		printf("%d %d %sppppp\n", inf1->x, inf1->y, inf1->string);
	}

	InfoR * inf2 = search_elements1_table(table, "n", 0);
	if(inf2){
		for(int i = 0; inf2[i].info.string; i++){
			printf(" %d %d  %d  %s\n", inf2[i].release, inf2[i].info.x, inf2[i].info.y, inf2[i].info.string);
		}
	}

	Info * inf3 = search_elements2_table(table, 2, 0);
	if(inf3){
			printf(" %d %d   %s\n", inf3->x, inf3->y, inf3->string);
		
	}

	print_table(table);



	return 0;
}