
//
// Implementation of a HashTable that stores void *
//

#include "HashTableVoid.h"

// Obtain the hash code of a key
int HashTableVoid::hash(const char * key) {
    int sum = 0;

    while (*key) {
	sum += *key;
	key++;
    }

    return sum % TableSize;
}

// Constructor for hash table. Initializes hash table
HashTableVoid::HashTableVoid() {
    _buckets = (HashTableVoidEntry **)malloc(TableSize * sizeof(HashTableVoidEntry*));

    for (int i = 0; i < TableSize; i++) {
	_buckets[i] = NULL;
    }       
}

// Add a record to the hash table. Returns true if key already exists.
// Substitute content if key already exists.
bool HashTableVoid::insertItem(const char * key, void * data) {
    int h = hash(key);
    
    HashTableVoidEntry * e = _buckets[h];

    while (e != NULL) {
	if (!strcmp(e->_key, key)) {
	    e->_data = data;
	    return true;
	}
	
	e = e->_next;
    }

    e = new HashTableVoidEntry;
    e->_key = strdup(key);
    e->_data = data;
    e->_next = _buckets[h];
    _buckets[h] = e;
    return false;
}

// Find a key in the dictionary and place in "data" the corresponding record
// Returns false if key is does not exist
bool HashTableVoid::find(const char * key, void ** data) {
    int h = hash(key);

    HashTableVoidEntry * e = _buckets[h];

    while (e != NULL) {
	if (!strcmp(e->_key, key)) {
	    *data = e->_data;
	    return true;
	}

	e = e->_next;
    }

    return false;
}

// Removes an element in the hash table. Return false if key does not exist.
bool HashTableVoid::removeElement(const char * key) {
    int h = hash(key);

    HashTableVoidEntry * e = _buckets[h];
    HashTableVoidEntry * prev = NULL;

    while (e != NULL) {
	if (!strcmp(e->_key, key)) {
	    if (prev != NULL) {
		prev->_next = e->_next;
	    }
	    else {
	        _buckets[h] = e->_next;
	    }

	    //free(e->_key);
	    delete e;
	    return true;
	}

	prev = e;
	e = e->_next;	
    }

    return false;
}

// Creates an iterator object for this hash table
HashTableVoidIterator::HashTableVoidIterator(HashTableVoid * hashTable) {
    _hashTable = hashTable;
    _currentBucket = 0;
    _currentEntry = _hashTable->_buckets[_currentBucket];
}

// Returns true if there is a next element. Stores data value in data.
bool HashTableVoidIterator::next(const char * & key, void * & data) {
    for (int i = _currentBucket; i < 2039; i++) {
	HashTableVoidEntry * _bucket = _currentEntry;

	if (_bucket != NULL) {
	    key = _bucket->_key;
	    data = _bucket->_data;


	    if (_bucket->_next != NULL) {
		_currentEntry = _bucket->_next;
	    }	
	    else {
		_currentBucket = ++i;
		_currentEntry = _hashTable->_buckets[_currentBucket];
	    }

	    return true;
	}
	else {
	    _currentBucket = i+1;
	    _currentEntry = _hashTable->_buckets[_currentBucket];
	}
    }


    return false;
}

