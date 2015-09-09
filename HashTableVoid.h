
//
// Hash Table
//

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Each hash entry stores a key, object pair
struct HashTableVoidEntry {
  const char * _key;
  void * _data;
  HashTableVoidEntry * _next;
};

struct ChatRoom {
  const char * _key;
  const char * _user;
  const char * _message;
  ChatRoom * _next;
};

struct UserInfo {
  const char * _key;
  const char * _room;
  UserInfo * _next;
};

// This is a Hash table that maps string keys to objects of type Data
class HashTableVoid {
 public:
  // Number of buckets
  enum { TableSize = 2039};
  
  // Array of the hash buckets.
  HashTableVoidEntry **_buckets;

  // Array of the chat buckets.
  ChatRoom ** _chatBuckets;
  
  // Array of the user buckets.
  UserInfo ** _userBuckets;

  // Obtain the hash code of a key
  int hash(const char * key);
  
 public:
  HashTableVoid();
  
  // Add a record to the hash table. Returns true if key already exists.
  // Substitute content if key already exists.
  bool insertItem( const char * key, void * data);

  bool insertChat(const char * key, const char * user, const char * message);

  // Find a key in the dictionary and place in "data" the corresponding record
  // Returns false if key is does not exist
  bool find( const char * key, void ** data);

  // Removes an element in the hash table. Return false if key does not exist.
  bool removeElement(const char * key);
};

class HashTableVoidIterator {
  int _currentBucket;
  HashTableVoidEntry *_currentEntry;
  HashTableVoid * _hashTable;
 public:
  HashTableVoidIterator(HashTableVoid * hashTable);
  bool next(const char * & key, void * & data);
};

