#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int ADDR_LEN = 24; //16M bytes = 2^4 * 2^20 = 2^24 B
int CACHE_SIZE;
int ASSOC;
int BLOCK_SIZE;

/*
#sets = CACHE_SIZE / BLOCK_SIZE / ASSOC

offset_bits = log2(BLOCK_SIZE)
index_bits = log2(#sets)
tag_bits = ADDR_LEN - offset_bits - index_bits

[                24 bits                ]
[ tag_bits ][ index_bits ][ offset_bits ]


*/

int log2(int n) {
    int r=0;
    while (n>>=1) r++;
    return r;
}

void memcpyn (char* dest, char* src, int bytes) {
    for (int i = 0;i < bytes;i++) {
        dest[i] = src[bytes-1-i];
    }
}

struct wayNode {
    int way;
    struct wayNode* prev;
    struct wayNode* next;
};


int main(int argc, char* argv[]) {
    if (argc < 5) {
        printf("Not enough arguments");
        exit(1);
    }

    FILE *file= fopen(argv[1], "r");

    CACHE_SIZE = strtol(argv[2], (char**) NULL, 10) * 1024;  //in Bytes
    ASSOC = strtol(argv[3], (char**) NULL, 10);
    BLOCK_SIZE = strtol(argv[4], (char**) NULL, 10);

    int num_blocks = (int)CACHE_SIZE/BLOCK_SIZE;
    int num_sets = (int)num_blocks/ASSOC;

    int offset_bits = log2(BLOCK_SIZE);
    int index_bits = log2(CACHE_SIZE / BLOCK_SIZE / ASSOC);
    int tag_bits = ADDR_LEN - offset_bits - index_bits;

    
    char* memory = (char*) calloc(1, (1<<ADDR_LEN) * sizeof(char));  //access by memory[byte_address]
    /*
    Cache blocks:
    BLOCK_SIZE bytes
    4 index bytes
    1 valid byte
    */

    /*
    cache_ways[i][0 _ index _ offset]
    cache_ways_tag[i][0 _ index]
    cache_ways_valid[i][0 _ index]
    */
    char** cache_ways = (char**) calloc(1,ASSOC * sizeof(char*));
    int** cache_ways_tag = (int**) calloc(1,ASSOC * sizeof(int*));
    char** cache_ways_valid = (char**) calloc(1,ASSOC * sizeof(char*));
    

    //Array of wayNode pointers
    struct wayNode** cache_start_waynode = (struct wayNode**) calloc(1, num_sets * sizeof(struct wayNode*));
    struct wayNode** cache_end_waynode = (struct wayNode**) calloc(1, num_sets * sizeof(struct wayNode*));

    for (int i = 0;i < num_sets;i++) {
        cache_start_waynode[i] = (struct wayNode*) calloc(1, sizeof(struct wayNode));
        struct wayNode* curr_node = cache_start_waynode[i];
        curr_node->way = 0;
        for (int j = 1;j < ASSOC;j++) {
            curr_node->next = (struct wayNode*) calloc(1, sizeof(struct wayNode));
            curr_node->next->prev = curr_node;
            curr_node = curr_node->next;
            curr_node->way = j;
        }
        cache_end_waynode[i] = curr_node;
    }

    for (int i = 0;i < ASSOC;i++) {
        cache_ways[i] = (char*) calloc(1, num_sets * (BLOCK_SIZE) * sizeof(char));
        cache_ways_tag[i] = (int*) calloc(1, num_sets * sizeof(int));
        for (int j = 0;j < num_sets;j++) {
            cache_ways_tag[i][j] = -1;
        }
        cache_ways_valid[i] = (char*) calloc(1, num_sets * sizeof(char));
    }

    /*
    Access by:
        Block content
            cache[index_mask(address) * num_blocks +  + offset]
    */

    char optype[10];
    int address;
    int bytes;
    long value;

    // remember replacement heuristic!!!!!!!!!!!!
    // 
    while (fscanf(file, "%s %x %d %lx", optype, &address, &bytes, &value) != EOF){
        // printf("\n\n\n");
        char hit = 0;
        long result = 0;
        int TAG_MASK = (1<<(index_bits + offset_bits))-1;
        int OFFSET_MASK = ((1<<(tag_bits+index_bits))-1)<<offset_bits;
        int index_offset = address & TAG_MASK;
        int index = index_offset>>offset_bits;
        int tag_index_0 = address & OFFSET_MASK;
        int index_0 = tag_index_0 & TAG_MASK;
        int tag = address & (~TAG_MASK);
        // printf("address: %x, bytes: %d, value: %lx, sizeof(value): %d\n", address, bytes, value, sizeof(value));
        // printf("index_offset: %x, index: %x, tag: %x\n", index_offset, index, tag);

        // printf("INDEX: %d\n", index);
        // printf("\nCACHE at index: ");
        // for (int i = 0;i < ASSOC;i++) {
        //     printf(" cache at way %d: %x", i, *(cache_ways[i] + index_offset));
        // }
        // printf("\n");
        if (strcmp(optype, "store") == 0) {
            /*
            Search cache_ways_tag and cach_ways_valid
                If match
                    Change value in cache_ways
                    Hit = true
                Else
                    Use replacement heuristic to get way
                    Store value in cache_ways[way]
                    cache_ways_valid[way][index] = true
                    cache_ways_tag[way][index] = tag
                Change value in memory
                Print "store" address "miss"/"hit"
            */
            for (int i = 0;i < ASSOC;i++) {
                if (cache_ways_valid[i][index] && cache_ways_tag[i][index] == tag) {
                    memcpyn(cache_ways[i]+index_offset, (char*)&value, bytes);
                    hit = 1;
                    break;
                }
            }
            if (!hit) {  //miss
                // struct wayNode* curr_node = cache_start_waynode[index];
                // int way = curr_node->way;
                // if (curr_node->next) {
                //     cache_start_waynode[index] = curr_node->next;
                //     curr_node->next->prev = NULL;
                //     curr_node->next = NULL;
                //     cache_end_waynode[index]->next = curr_node;
                //     curr_node->prev = cache_end_waynode[index];
                //     cache_end_waynode[index] = curr_node;
                // }

                // cache_ways_valid[way][index] = 1;
                // cache_ways_tag[way][index] = tag;
            }
            else {  //hit
                struct wayNode* curr_node = cache_start_waynode[index];
                for (int i = 0;i < ASSOC;i++) {
                    if (cache_ways_tag[curr_node->way][index] == tag) {
                        break;
                    }
                    curr_node = curr_node->next;
                }
                if (curr_node->next) {
                    if (curr_node->prev) {
                        curr_node->prev->next = curr_node->next;
                    }
                    if (curr_node == cache_start_waynode[index]) {
                        cache_start_waynode[index] = curr_node->next;
                    }
                    curr_node->next->prev = curr_node->prev;
                    curr_node->next = NULL;
                    curr_node->prev = cache_end_waynode[index];
                    cache_end_waynode[index]->next = curr_node;
                    cache_end_waynode[index] = curr_node;
                }
            }
            memcpyn(memory + address, (char*)&value, bytes);
            memcpyn((char*)&result, memory + 1, 1);
            if (hit) {
                printf("%s 0x%x hit\n", optype, address);
            }
            else {
                printf("%s 0x%x miss\n", optype, address);
            }
        }
        else if (strcmp(optype, "load") == 0) {
            /*
            Search cache_ways_tag and cach_ways_valid
                If match
                    Memcpy x Bytes cache value to result
                    Hit = true
                Else
                    Use replacement heuristic to get way
                    Bring whole block into cache_ways[way]
                    cache_ways_valid[way][index] = true
                    cache_ways_tag[way][index] = tag

                    Memcpy x Bytes cache_ways[way][index_offset] value to result
                Print "load" address "miss"/"hit" value
            */
            for (int i = 0;i < ASSOC;i++) {
                if (cache_ways_valid[i][index] && cache_ways_tag[i][index] == tag) {
                    memcpyn((char*)&result, cache_ways[i]+index_offset, bytes);
                    hit = 1;
                    break;
                }
            }
            if (!hit) {  //miss
                // printf("START NODE: %d\n", cache_start_waynode[index]->way);
                struct wayNode* curr_node = cache_start_waynode[index];
                int way = curr_node->way;
                if (curr_node->next) {
                    cache_start_waynode[index] = curr_node->next;
                    curr_node->next->prev = NULL;
                    curr_node->next = NULL;
                    cache_end_waynode[index]->next = curr_node;
                    curr_node->prev = cache_end_waynode[index];
                    cache_end_waynode[index] = curr_node;
                }

                memcpy(cache_ways[way]+index_0, memory + tag_index_0, BLOCK_SIZE);  //!!!!!!!!!!!!
                cache_ways_valid[way][index] = 1;
                cache_ways_tag[way][index] = tag;
                memcpyn((char*)&result, cache_ways[way]+index_offset, bytes);
            }
            else {  //hit
                struct wayNode* curr_node = cache_start_waynode[index];
                for (int i = 0;i < ASSOC;i++) {
                    if (cache_ways_tag[curr_node->way][index] == tag) {
                        // printf("FOUND WAY WITH SAME TAG: %d\n", curr_node->way);
                        break;
                    }
                    curr_node = curr_node->next;
                }
                if (curr_node->next) {
                     if (curr_node->prev) {
                        curr_node->prev->next = curr_node->next;
                    }
                    if (curr_node == cache_start_waynode[index]) {
                        cache_start_waynode[index] = curr_node->next;
                    }
                    curr_node->next->prev = curr_node->prev;
                    
                    curr_node->next = NULL;
                    curr_node->prev = cache_end_waynode[index];
                    cache_end_waynode[index]->next = curr_node;
                    cache_end_waynode[index] = curr_node;
                }
            }
            if (hit) {
                printf("%s 0x%x hit ", optype, address);
            }
            else {
                printf("%s 0x%x miss ", optype, address);
            }
            for (int i = bytes-1;i >= 0;i--) {
                printf("%02hhx", *(((char*)(&result))+i));
            }
            printf("\n");
        }
        // printf("NODES: ");
        // struct wayNode* curr = cache_start_waynode[index];
        // while (curr) {
        //     printf(" way %d", curr->way);
        //     curr = curr->next;
        // }
    }
    
    free(memory);
    for (int i = 0;i < ASSOC;i++) {
        free(cache_ways[i]);
        free(cache_ways_tag[i]);
        free(cache_ways_valid[i]);
    }
    free(cache_ways);
    free(cache_ways_tag);
    free(cache_ways_valid);
    exit(0);
}