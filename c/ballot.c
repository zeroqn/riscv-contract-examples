#include <stdlib.h>
#include <string.h>

#include "ballot.h"
#include "pvm.h"
#include "pvm_extend.h"

// Constants
// Use const uint8_t so that we can easily pvm_save and pvm_ret
const uint8_t HAS_VOTER_RIGHT = 1;
const uint8_t NO_RIGHT = 2;

// Vote value
const uint8_t NONE = 0;
const uint8_t YEA = 1;
const uint8_t NAY = 2;

// Result
const uint8_t APPROVED = 1;
const uint8_t FAILED = 2;

int main(int argc, char *argv[]) {
  if (argc == 1) {
    return SUCCESS;
  }

  uint8_t *owner_key = (uint8_t *)OWNER_KEY;
  uint8_t *topic_key = (uint8_t *)TOPIC_KEY;
  uint8_t *state_key = (uint8_t *)STATE_KEY;
  uint8_t *total_voter_key = (uint8_t *)TOTAL_VOTER;
  uint8_t *yea_count_key = (uint8_t *)YEA_COUNT_KEY;

  addr_t owner;
  topic_t topic;
  uint8_t state;
  uint64_t total_voter = 0;
  uint64_t yea_count = 0;

  // ### Initialize state

  // Load contract owner
  pvm_load(owner_key, strlen(OWNER_KEY), owner.addr, ADDRESS_SIZE, NULL);

  // Load topic
  pvm_load(topic_key, strlen(TOPIC_KEY), (uint8_t *)topic.str, MAX_TOPIC_LEN,
           NULL);

  // Load state value
  uint8_t sv[1] = {0};
  pvm_load(state_key, strlen(STATE_KEY), sv, 1, NULL);
  state = sv[0];

  // Load voter count
  uint8_t vc[8] = {0};
  pvm_load(total_voter_key, strlen(TOTAL_VOTER), vc, 8, NULL);
  total_voter = *vc;

  // Load yea count
  uint8_t yc[8] = {0};
  pvm_load(yea_count_key, strlen(YEA_COUNT_KEY), yc, 8, NULL);
  yea_count = *yc;

  // ### Contract APIs

  if (strcmp(argv[1], SET_OWNER) == 0) {
    if (argc != 2) {
      return ERR_METHOD_ARG_NUM;
    }

    if (FALSE == is_empty(owner.addr, ADDRESS_SIZE)) {
      return ERR_OWNER_ALREADY_SET;
    }

    addr_t caller;
    pvm_caller(caller.addr);

    pvm_save(owner_key, strlen(OWNER_KEY), caller.addr, ADDRESS_SIZE);

    return SUCCESS;
  }

  // Return owner address
  else if (strcmp(argv[1], GET_OWNER) == 0) {
    if (argc != 2) {
      return ERR_METHOD_ARG_NUM;
    }

    if (TRUE == is_empty(owner.addr, ADDRESS_SIZE)) {
        return ERR_OWNER_NO_SET;
    }

    pvm_ret(owner.addr, ADDRESS_SIZE);
    return SUCCESS;
  }

  if (TRUE == is_empty(owner.addr, ADDRESS_SIZE)) {
    pvm_debug("no owner");
    return ERR_OWNER_NO_SET;
  }

  // Return ballot topic
  if (strcmp(argv[1], GET_TOPIC) == 0) {
    if (argc != 2) {
      return ERR_METHOD_ARG_NUM;
    }

    if (TRUE == is_empty((uint8_t *)topic.str, strlen(topic.str))) {
      return ERR_TOPIC_NO_SET;
    }

    pvm_ret((uint8_t *)topic.str, strlen(topic.str));
    return SUCCESS;
  }

  // Set ballot topic
  else if (strcmp(argv[1], SET_TOPIC) == 0) {
    if (argc != 3) {
      return ERR_METHOD_ARG_NUM;
    }

    if (FALSE == is_empty((uint8_t *)topic.str, strlen(topic.str))) {
      return ERR_TOPIC_ALREADY_SET;
    }

    addr_t caller;
    pvm_caller(caller.addr);

    if (FALSE == is_owner(caller)) {
      return ERR_NO_PERMISSION;
    }

    char *topic = argv[2];
    if (MAX_TOPIC_LEN < strlen(topic)) {
      return ERR_EXCEE_MAX_LEN;
    }

    pvm_save(topic_key, strlen(TOPIC_KEY), (uint8_t *)topic, strlen(topic));
    return SUCCESS;
  }

  // Authorize voter right to given address
  else if (strcmp(argv[1], AUTHORIZE_VOTE_RIGHT) == 0) {
    if (argc != 3) {
      return ERR_METHOD_ARG_NUM;
    }

    addr_t caller;
    pvm_caller(caller.addr);

    if (FALSE == is_owner(caller)) {
      return ERR_NO_PERMISSION;
    }

    if (total_voter == MAX_VOTER_COUNT) {
      return ERR_MAX_VOTER_REACHED;
    }

    // Address is passed by hex encoded
    if (40 != strlen(argv[2])) {
      return ERR_INVALID_ADDRESS;
    }

    addr_t user;
    pvm_hex2bin(argv[2], (char *)user.addr);

    voter_t voter;
    voter.state[0] = HAS_VOTER_RIGHT;

    pvm_save(user.addr, ADDRESS_SIZE, voter.state, sizeof(voter.state));

    ++total_voter;
    pvm_save(total_voter_key, strlen(TOTAL_VOTER), (uint8_t *)&total_voter, 8);

    return SUCCESS;
  }

  // Check whether caller has voter right
  else if (strcmp(argv[1], IS_VOTER) == 0) {
    if (argc != 2) {
      return ERR_METHOD_ARG_NUM;
    }

    addr_t caller;
    pvm_caller(caller.addr);

    if (FALSE == has_vote_right(caller)) {
      pvm_ret((uint8_t *)&NO_RIGHT, 1);
    } else {
      pvm_ret((uint8_t *)&HAS_VOTER_RIGHT, 1);
    }

    return SUCCESS;
  }

  // Start vote phrase
  else if (strcmp(argv[1], START_VOTE) == 0) {
    if (argc != 2) {
      return ERR_METHOD_ARG_NUM;
    }

    if (PREPARE != state) {
      return ERR_VOTE_ALREADY_STARTED_OR_END;
    }

    addr_t caller;
    pvm_caller(caller.addr);

    if (FALSE == is_owner(caller)) {
      return ERR_NO_PERMISSION;
    }

    uint8_t started_state[1] = {STARTED};
    pvm_save(state_key, strlen(STATE_KEY), started_state, 1);

    return SUCCESS;
  }

  // End vote phrase
  else if (strcmp(argv[1], END_VOTE) == 0) {
    if (argc != 2) {
      return ERR_METHOD_ARG_NUM;
    }

    if (STARTED != state) {
      return ERR_VOTE_NOT_STARTED_YET;
    }

    addr_t caller;
    pvm_caller(caller.addr);

    if (FALSE == is_owner(caller)) {
      return ERR_NO_PERMISSION;
    }

    uint8_t end_state[1] = {END};
    pvm_save(state_key, strlen(STATE_KEY), end_state, 1);

    return SUCCESS;
  }

  // Vote yea or nay to ballot topic
  else if (strcmp(argv[1], VOTE) == 0) {
    if (argc != 3) {
      return ERR_METHOD_ARG_NUM;
    }

    if (STARTED != state) {
      return ERR_VOTE_NOT_STARTED_YET;
    }

    uint8_t vote;
    if (strcmp(argv[2], YEA_STR)) {
      vote = YEA;
    } else if (strcmp(argv[2], NAY_STR)) {
      vote = NAY;
    } else {
      return ERR_INVALID_VOTE_VALUE;
    }

    addr_t caller;
    pvm_caller(caller.addr);

    if (FALSE == has_vote_right(caller)) {
      return ERR_NO_PERMISSION;
    }

    voter_t voter;
    pvm_load(caller.addr, ADDRESS_SIZE, voter.state, sizeof(voter.state), NULL);

    if (NONE != voter.state[1]) {
      return ERR_ALREADY_VOTED;
    }

    voter.state[1] = vote;
    pvm_save(caller.addr, ADDRESS_SIZE, voter.state, sizeof(voter.state));

    if (vote == YEA) {
      ++yea_count;

      pvm_save(yea_count_key, strlen(YEA_COUNT_KEY), (uint8_t *)&yea_count, 8);
    }

    return SUCCESS;
  }

  else if (strcmp(argv[1], ANN_RESULT) == 0) {
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

    return SUCCESS;
  }

  return SUCCESS;
}

int is_empty(uint8_t *val, uint64_t max_size) {
  if (max_size > 1024) {
    return ERR_EXCEE_MAX_LEN;
  }

  for (int i = 0; i < max_size; ++i) {
    if (val[i] != 0) {
      return FALSE;
    }
  }

  return TRUE;
}

int caller_mached(addr_t caller, addr_t expect) {
  for (int i = 0; i < ADDRESS_SIZE; ++i) {
    if (caller.addr[i] != expect.addr[i]) {
      return FALSE;
    }
  }

  return TRUE;
}

int is_owner(addr_t owner) {
  addr_t caller;
  pvm_caller(caller.addr);

  return caller_mached(caller, owner);
}

int has_vote_right(addr_t caller) {
  voter_t voter;
  pvm_load(caller.addr, ADDRESS_SIZE, voter.state, sizeof(voter.state), NULL);

  if (HAS_VOTER_RIGHT == voter.state[0]) {
    return TRUE;
  } else {
    return FALSE;
  }
}

int already_voted(addr_t caller) {
  voter_t voter;
  pvm_load(caller.addr, ADDRESS_SIZE, voter.state, sizeof(voter.state), NULL);

  if (NONE != voter.state[1]) {
    return TRUE;
  } else {
    return FALSE;
  }
}
