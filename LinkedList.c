#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "LinkedList.h"

//
// Initialize a linked list
//
void llist_init(LinkedList * list) {
	list->head = NULL;
}

//
// It prints the elements
//
void llist_print(LinkedList * list) {
	
	ListNode * e;

	if (list->head == NULL) {
		printf("EMPTY\n");
		return;
	}

	e = list->head;
	while (e != NULL) {
		printf("%s\n", e->value);
		e = e->next;
	}
	printf("\n");
}

void llist_printToUser(LinkedList * list, int fd) {
	ListNode * e;

	if (list->head == NULL) {
		return;
	}

	e = list->head;
	while (e != NULL) {
		const char * temp = e->value;
		write(fd, temp, strlen(temp));
		write(fd, "\r\n", strlen("\r\n"));
		e = e->next;
	}

	return;
}

void llist_printToUserMessages(LinkedList * list, int fd, int pos) {
	ListNode * e;

	if (pos >= 100) {
	    write(fd, "NO-NEW-MESSAGES\r\n", strlen("NO-NEW-MESSAGES\r\n"));
	    return;
	}

	if (list->head == NULL) {
	    write(fd, "ERROR (User not in room)\r\n", strlen("ERROR (User not in room)\r\n"));
	    return;
	}

	e = list->head;

	if (llist_number_elements(list) <= pos) {
	    write(fd, "NO-NEW-MESSAGES\r\n", strlen("NO-NEW-MESSAGES\r\n"));
	    return;
	}

	for (int i = 0; i < pos; i++) {
	    if (e->next != NULL) {
		e = e->next;
	    }
	    else {
		write(fd, "NO-NEW-MESSAGES\r\n", strlen("NO-NEW-MESSAGES\r\n"));
		return;
	    }
	}

	while (e != NULL) {
		const char * temp = e->value;
		write(fd, temp, strlen(temp));
		e = e->next;
	}

	write(fd, "\r\n", strlen("\r\n"));
}

//
// Appends a new node with this value at the beginning of the list
//
void llist_add(LinkedList * list, char * value) {
	// Create new node
	ListNode * n = (ListNode *) malloc(sizeof(ListNode));
	n->value = strdup(value);
	
	// Add at the beginning of the list
	n->next = list->head;
	list->head = n;
}

//
// Returns true if the value exists in the list.
//
int llist_exists(LinkedList * list, char * value) {
	ListNode * n = list->head;
	char * v = value;


	while (n != NULL) {
	    if (!strcmp(n->value, v)) {
		return 1;
	    }

	    n = n->next;
	}

	return 0;
}

//
// It removes the entry with that value in the list.
//
int llist_remove(LinkedList * list, char * value) {
	ListNode * n1 = list->head;
	ListNode * n2 = NULL;
	char * v = value;

	if (!strcmp(n1->value, v)) {
	    list->head = n1->next;
	    return 1;
	}

	n2 = n1;
	n1 = n1->next;

	while (n1 != NULL) {
	    if (!strcmp(n1->value, v)) {
		n2->next = n1->next;
		return 1;	
	    }

	    n2 = n1;
	    n1 = n1->next;
	}	

	return 0;
}

//
// It stores in *value the value that correspond to the ith entry.
// It returns 1 if success or 0 if there is no ith entry.
//
int llist_get_ith(LinkedList * list, int ith, char * value) {
	ListNode * n = list->head;
	int i = 0;

	while (i < ith && n->next != NULL) {
	    n = n->next;
	    i++;
	}
	
	if (n->next == NULL && ith > i) {
	    return 0;
	}

	value = strdup(n->value);

	return 1;
}

//
// It removes the ith entry from the list.
// It returns 1 if success or 0 if there is no ith entry.
//
int llist_remove_ith(LinkedList * list, int ith) {
	ListNode * n1 = list->head;
	ListNode * n2 = NULL;
	int i = 0;

	if (ith == 1) {
	    n1 = n1->next;
	    return 1;
	}

	n2 = list->head;

	while (i < ith && n1->next != NULL) {
	    n2 = n1;
	    n1 = n1->next;
	    i++;
	}

	if (n1->next == NULL) {
	    return 0;
	}

	n2->next = n1->next;

	return 1;
}

//
// It returns the number of elements in the list.
//
int llist_number_elements(LinkedList * list) {
	ListNode * n = list->head;
	int count = 0;

	if (list->head == NULL) {
	    return 0;
	}

	count = 1;
		
	while (n->next != NULL) {
	    n = n->next;
	    count++;
	}

	return count;
}


//
// It saves the list in a file called file_name. The format of the
// file is as follows:
//
// value1\n
// value2\n
// ...
//
int llist_save(LinkedList * list, char * file_name) {
	FILE * f = fopen(file_name, "w");
	ListNode * n = list->head;

	if (f == NULL) {
	    return 0;
	}

	while (n != NULL) {
	    fprintf(f, "%s\n", n->value);
	    n = n->next;
	}

	fclose(f);

	return 0;
}

//
// It reads the list from the file_name indicated. If the list already has entries, 
// it will clear the entries.
//
int llist_read(LinkedList * list, char * file_name) {
	FILE * f = fopen(file_name, "r");
	ListNode * n = list->head;
	char * val;

	if (f == NULL) {
	    return 0;
	}

	while (fscanf(f, "%s", val) == 1) {
	    llist_add(list, val);
	    
	}

	return 1;
}


//
// It sorts the list. The parameter ascending determines if the
// order is ascending (1) or descending(0).
//
void llist_sort(LinkedList * list, int ascending) {
	ListNode * n1 = list->head;

	if (n1 == NULL) {
	    return;
	}

	ListNode * n2 = n1->next;
	char * temp;

	if (ascending == 1) {
	    while (n1->next != NULL) {
		for (n2 = n1->next; n2 != NULL; n2 = n2->next) {
		    if (strcmp(n1->value, n2->value) > 0) {
			temp = strdup(n1->value);
			n1->value = strdup(n2->value);
			n2->value = strdup(temp);	
		    }
		}

		n1 = n1->next;
	    }
	}

	if (ascending == 0) {
	    while (n1->next != NULL) {
		for (n2 = n1->next; n2 != NULL; n2 = n2->next) {
		    if (n2->value > n1->value) {
			temp = n1->value;
			n1->value = n2->value;
			n2->value = temp;
		    }
		}

		n1 = n1->next;
	    }
	}
}

//
// It removes the first entry in the list and puts value in *value.
// It also frees memory allocated for the node
//
int llist_remove_first(LinkedList * list, char * value) {
	ListNode * n1 = list->head;	
	
	if (n1 == NULL) {
	    return 0;
	}

	llist_get_ith(list, 0, value);	
	list->head = n1->next;
	free(n1);

	return 1;
}

int llist_remove_first(LinkedList * list) {
	ListNode * n1 = list->head;

	if (n1 == NULL) {
	    return 0;
	}

	list->head = n1->next;
	free(n1);
	
	return 1;
}

//
// It removes the last entry in the list and puts value in *value/
// It also frees memory allocated for node.
//
int llist_remove_last(LinkedList * list, char * value) {
	ListNode * n1 = list->head;
	ListNode * n2;
	int count = llist_number_elements(list);

	if (n1 == NULL) {
	    return 0;
	}

	llist_get_ith(list, count - 1, value);

	while ((n1->next)->next != NULL) {
	    n1 = n1->next;
	}

	n2 = n1->next;
	n1->next = NULL;
	
	free(n2);

	return 1;
}

//
// Insert a value at the beginning of the list.
//
void llist_insert_first(LinkedList * list, char * value) {
	llist_add(list, strdup(value));
}

//
// Insert a value at the end of the list.
//
void llist_insert_last(LinkedList * list, char * value) {
	ListNode * n;

	if (list->head != NULL) {
	    n = list->head;
	    if (n->next == NULL) {
		printf("%s", "WTF");
	    }

	    while (n->next != NULL) {
	        n = n->next;
	    }
	}

	ListNode * last = (ListNode *)malloc(sizeof(ListNode));
	last->value = strdup(value);
	
	// Add at the end of the list
	last->next = NULL;

	if (list->head != NULL) {
	    n->next = last;
	}
	else {
	    list->head = last;
	}
}

//
// Clear all elements in the list and free the nodes
//
void llist_clear(LinkedList * list) {
	ListNode * n = list->head;
	char * temp;

	while (list->head != NULL) {
	    llist_remove_first(list, temp); 
	}

	list->head = NULL;
}

