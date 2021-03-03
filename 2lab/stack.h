#ifndef LAB2_H
#define LAB2_H

typedef struct CONTEINER CONTEINER;

typedef struct{
	CONTEINER* conteiner;
}STACK;

void stack_init(STACK* Stack, int size, int element_size);
int stack_push(STACK* Stack, void* operand);
void* stack_pop(STACK* Stack);
void stack_clear(STACK* Stack);

#endif //LAB2_H









/*
typedef struct{
	char data[STACK_SIZE];
	int top;
}STACK_VECT;

typedef struct ITEM{
	char data;
	struct ITEM* next;
}ITEM;

typedef struct{
	ITEM* top;
}STACK_LIST;
*/