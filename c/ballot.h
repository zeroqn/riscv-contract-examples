#include <stdint.h>

#ifndef _BALLOT_H
#define _BALLOT_H

// Common size
#define ADDRESS_SIZE 20
#define MAX_TOPIC_LEN 100
#define MAX_VOTER_COUNT 1 << 30

// Common bool
#define TRUE 0
#define FALSE 1

// Common keys
#define OWNER_KEY "owner_key"
#define TOPIC_KEY "topic_key"
#define STATE_KEY "state_key"
#define TOTAL_VOTER "total_voter"
#define YEA_COUNT_KEY "yea_count_key"

#define YEA_STR "1"
#define NAY_STR "2"

// Ballot state
#define PREPARE 0
#define STARTED 1
#define END 2

// Return
#define SUCCESS 0

// Errors
#define ERR_METHOD_ARG_NUM 100
#define ERR_OWNER_NO_SET 101
#define ERR_OWNER_ALREADY_SET 102
#define ERR_TOPIC_NO_SET 103
#define ERR_TOPIC_ALREADY_SET 104
#define ERR_NO_PERMISSION 105
#define ERR_EXCEE_MAX_LEN 106
#define ERR_INVALID_VOTE_VALUE 107
#define ERR_MAX_VOTER_REACHED 108
#define ERR_VOTE_ALREADY_STARTED_OR_END 108
#define ERR_VOTE_NOT_STARTED_YET 109
#define ERR_VOTE_NOT_END_YET 110
#define ERR_ALREADY_VOTED 111
#define ERR_INVALID_ADDRESS 112

// API names
#define SET_OWNER "set_owner" // We don't have constructor yet
#define GET_OWNER "get_owner"
#define GET_TOPIC "get_topic"
#define SET_TOPIC "set_topic"
#define AUTHORIZE_VOTE_RIGHT "authorize_vote_right"
#define IS_VOTER "is_voter"
#define START_VOTE "start_vote"
#define END_VOTE "end_vote"
#define VOTE "vote"
#define ANN_RESULT "ann_result"

// Types
typedef struct addr_t {
  uint8_t addr[ADDRESS_SIZE];
} addr_t;
typedef struct topic_t {
  char str[MAX_TOPIC_LEN];
} topic_t;

// Voter
// First u8: 1 means has voter right
// Second u8: 1 means yea, 2 means nay
typedef struct voter_t {
  uint8_t state[2];
} voter_t;

// Contract helpers
int is_empty(uint8_t *val, uint64_t max_size);
int addr_mached(addr_t caller, addr_t expect);
int is_owner(addr_t owner_addr);
int has_vote_right(addr_t caller);
int already_voted(addr_t caller);

#endif
