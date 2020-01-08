#include <string.h>
#include <stdlib.h>

#include "pvm.h"
#include "pvm_extend.h"
#include "helper.h"

// Common data
const uint8_t MAX_TOPIC_SIZE = 100;
const uint64_t MAX_VOTER_SIZE = 1 << 30;

// Common keys
const char *OWNER_KEY = "owner_key";
const char *TOPIC_KEY = "topic_key";
const char *STATE_KEY = "state_key";
const char *TOTAL_VOTER = "total_voter_key";
const char *YEA_COUNT_KEY = "yea_count_key";

// Ballot state
const uint8_t PREPARE = 0;
const uint8_t STARTED = 1;
const uint8_t END = 2;

// Ballot Result
const uint8_t APPROVED = 0;
const uint8_t FAILED = 1;

// Vote value
const uint8_t NAY = 0;
const uint8_t YEA = 1;
const uint8_t NONE = 2;

const char *NAY_STR = "0";
const char *YEA_STR = "1";

// Return
const int SUCCESS = 0;

// Errors
const int ERR_NO_KEY = 1;
const int ERR_METHOD_ARG_NUM = 2;
const int ERR_TOPIC_NO_SET = 3;
const int ERR_TOPIC_ALREADY_SET = 4;
const int ERR_NO_PERMISSION = 5;
const int ERR_TOPIC_EXCEE_MAX_SIZE = 6;
const int ERR_INVALID_VOTE_VALUE = 7;
const int ERR_MAX_VOTER_REACHED = 8;
const int ERR_VOTE_ALREADY_STARTED_OR_END = 9;
const int ERR_VOTE_NOT_STARTED_YET = 10;
const int ERR_VOTE_NOT_END_YET = 10;

// APIs
const char *SET_TOPIC = "set_topic";
const char *AUTHORIZE_VOTE_RIGHT = "authorize_vote_right";
const char *GET_OWNER = "get_owner";
const char *GET_TOPIC = "get_topic";
const char *IS_VOTER = "is_voter";
const char *VOTE = "vote";
const char *START_VOTE = "start_vote";
const char *END_VOTE = "end_vote";
const char *ANN_RESULT = "ann_result";

int is_owner(uint8_t* owner_addr) {
    uint8_t caller[ADDRESS_SIZE];
    pvm_caller(&caller[0]);

    return match_addr(&caller[0], &owner_addr[0]);
}

int has_vote_right(uint8_t* addr) {
    if (FALSE == is_valid_addr(addr)) {
        return ERR_INVALID_ADDRESS;
    }

    int ret;
    uint8_t vote[1];

    ret = pvm_load(&addr[0], ADDRESS_SIZE, &vote[0], 1, NULL);
    if (ERR_NO_KEY == ret) {
        return FALSE;
    } else {
        return TRUE;
    }
}

int main(int argc, char *argv[]) {
  if (argc == 1) {
    return SUCCESS;
  }

  uint8_t *owner_key = (uint8_t *)strstr(OWNER_KEY, OWNER_KEY);
  uint8_t *topic_key = (uint8_t *)strstr(TOPIC_KEY, TOPIC_KEY);
  uint8_t *state_key = (uint8_t *)strstr(STATE_KEY, STATE_KEY);
  uint8_t *total_voter_key = (uint8_t *)strstr(TOTAL_VOTER, TOTAL_VOTER);
  uint8_t *yea_count_key = (uint8_t *)strstr(YEA_COUNT_KEY, YEA_COUNT_KEY);

  uint8_t owner[ADDRESS_SIZE];
  uint8_t topic[MAX_TOPIC_SIZE];
  uint8_t state;
  uint64_t total_voter = 0;
  uint64_t yea_count = 0;

  // ### Initialize state

  // Load contract owner, ERR_NO_KEY means we are deploying contract,
  // set owner to contract deployer.
  int has_owner =
      pvm_load(&owner_key[0], ADDRESS_SIZE, &owner[0], ADDRESS_SIZE, NULL);

  if (ERR_NO_KEY == has_owner) {
    pvm_caller(&owner[0]);
    pvm_save(&owner_key[0], strlen(OWNER_KEY), &owner[0], ADDRESS_SIZE);
  }

  // Load topic
  int has_topic = pvm_load(&topic_key[0], strlen(TOPIC_KEY), &topic[0],
                       MAX_TOPIC_SIZE, NULL);

  // Load state value
  uint8_t sv[1];
  int has_state = pvm_load(&state_key[0], strlen(STATE_KEY), &sv[0], 1, NULL);

  if (ERR_NO_KEY == has_state) {
      sv[0] = PREPARE;
      pvm_save(&state_key[0], strlen(STATE_KEY), &sv[0], 1);
  }
  state = sv[0];

  // Load voter count
  uint8_t vc[8];
  int has_voter = pvm_load(&total_voter_key[0], strlen(TOTAL_VOTER), &vc[0], 8, NULL);

  if (SUCCESS == has_voter) {
      total_voter = *vc;
  }

  // Load yea count
  uint8_t yc[8];
  int has_yea = pvm_load(&yea_count_key[0], strlen(YEA_COUNT_KEY), &yc[0], 8, NULL);

  if (SUCCESS == has_yea) {
      yea_count = *yc;
  }


  // ### Contract APIs

  // Return owner address
  if (strcmp(argv[1], GET_OWNER) == 0) {
    if (argc != 2) {
      return ERR_METHOD_ARG_NUM;
    }

    pvm_ret(&owner[0], ADDRESS_SIZE);
    return SUCCESS;
  }

  // Return ballot topic
  else if (strcmp(argv[1], GET_TOPIC)) {
    if (argc != 2) {
      return ERR_METHOD_ARG_NUM;
    }

    if (ERR_NO_KEY == has_topic) {
      return ERR_TOPIC_NO_SET;
    }

    pvm_ret(&topic[0], strlen((char *)&topic[0]));
    return SUCCESS;
  }

  // Set ballot topic
  else if (strcmp(argv[1], SET_TOPIC)) {
    if (argc != 3) {
      return ERR_METHOD_ARG_NUM;
    }

    if (ERR_NO_KEY != has_topic) {
      return ERR_TOPIC_ALREADY_SET;
    }

    if (FALSE == is_owner(&owner[0])) {
        return ERR_NO_PERMISSION;
    }

    uint8_t *topic = (uint8_t *)argv[2];
    if (MAX_TOPIC_SIZE < ARRAY_LEN(&topic)) {
        return ERR_TOPIC_EXCEE_MAX_SIZE;
    }

    pvm_save(&topic_key[0], strlen(TOPIC_KEY), &topic[0], ARRAY_LEN(&topic));
    return SUCCESS;
  }

  // Authorize voter right to given address
  else if (strcmp(argv[1], AUTHORIZE_VOTE_RIGHT)) {
    if (argc != 3) {
        return ERR_METHOD_ARG_NUM;
    }

    if (FALSE == is_owner(&owner[0])) {
        return ERR_NO_PERMISSION;
    }

    uint8_t *auth_addr = (uint8_t *)argv[2];
    if (FALSE == is_valid_addr(&auth_addr[0])) {
        return ERR_INVALID_ADDRESS;
    }

    if (total_voter == MAX_VOTER_SIZE) {
        return ERR_MAX_VOTER_REACHED;
    }

    uint8_t vote[1] = {NONE};
    pvm_save(&auth_addr[0], ADDRESS_SIZE, &vote[0], 1);

    ++total_voter;
    pvm_save(&total_voter_key[0], strlen(TOTAL_VOTER), (uint8_t *)total_voter, 8);

    return SUCCESS;
  }

  // Check whether caller has voter right
  else if (strcmp(argv[1], IS_VOTER)) {
      if (argc != 2) {
          return ERR_METHOD_ARG_NUM;
      }

      uint8_t caller[ADDRESS_SIZE];
      pvm_caller(&caller[0]);

      int is_voter = has_vote_right(&caller[0]);
      if (ERR_NO_KEY == is_voter) {
          pvm_ret((uint8_t *)&FALSE, 1);
      } else {
          pvm_ret((uint8_t *)&TRUE, 1);
      }

      return SUCCESS;
  }

  // Vote yea or nay to ballot topic
  else if (strcmp(argv[1], VOTE)) {
      if (argc != 3) {
          return ERR_METHOD_ARG_NUM;
      }

      if (STARTED != state) {
          return ERR_VOTE_NOT_STARTED_YET;
      }

      uint8_t vote[1];
      if (strcmp(argv[2], YEA_STR)) {
          vote[0] = YEA;
      }
      else if (strcmp(argv[2], NAY_STR)) {
          vote[0] = NAY;
      }
      else {
          return ERR_INVALID_VOTE_VALUE;
      }

      uint8_t caller[ADDRESS_SIZE];
      pvm_caller(&caller[0]);

      if (FALSE == has_vote_right(&caller[0])) {
          return ERR_NO_PERMISSION;
      }

      pvm_save(&caller[0], ADDRESS_SIZE, &vote[0], 1);

      return SUCCESS;
  }

  // Start vote phrase
  else if (strcmp(argv[1], START_VOTE)) {
      if (argc != 2) {
          return ERR_METHOD_ARG_NUM;
      }

      if (PREPARE != state) {
          return ERR_VOTE_ALREADY_STARTED_OR_END;
      }

      if (FALSE == is_owner(&owner[0])) {
          return ERR_NO_PERMISSION;
      }

      uint8_t started_state[1] = {STARTED};
      pvm_save(&state_key[0], strlen(STATE_KEY), &started_state[0], 1);

      return SUCCESS;
  }

  // End vote phrase
  else if (strcmp(argv[1], END_VOTE)) {
      if (argc != 2) {
          return ERR_METHOD_ARG_NUM;
      }

      if (STARTED != state) {
          return ERR_VOTE_NOT_STARTED_YET;
      }

      if (FALSE == is_owner(&owner[0])) {
          return ERR_NO_PERMISSION;
      }

      uint8_t end_state[1] = {END};
      pvm_save(&state_key[0], strlen(STATE_KEY), &end_state[0], 1);

      return SUCCESS;
  }

  else if (strcmp(argv[1], ANN_RESULT)) {
      if (argc != 2) {
          return ERR_METHOD_ARG_NUM;
      }

      if (END != state) {
          return ERR_VOTE_NOT_END_YET;
      }

      if (yea_count > total_voter / 2) {
          pvm_ret((uint8_t *)&APPROVED, 1);
      } else {
          pvm_ret((uint8_t *)&FAILED, 1);
      }
  }

  return SUCCESS;
}
