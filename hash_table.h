#ifndef HASH_TABLE_H
#define HASH_TABLE_H

typedef struct CacheNode {
    char* url;
    long hit_count;
    struct CacheNode* next;
} CacheNode;

typedef struct HashTable {
    size_t size;
    CacheNode** buckets;
} HashTable;

HashTable* ht_create(size_t size);
void ht_insert(HashTable* ht, const char* url);
CacheNode* ht_get(HashTable* ht, const char* url);
void ht_destroy(HashTable* ht);
void ht_save_results(HashTable* ht, const char* filename);

#endif