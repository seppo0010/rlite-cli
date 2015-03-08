#include "../src/rdb2rldf.h"
#include "hirlite.h"

int main() {
    const char *src = "tests/dump.rdb";
    const char *target = "tests/rdb2rld-test.rld";
    if (rdb2rld(src, target) != 0) {
        return 1;
    }

    rliteContext *rl = rliteConnect(target, 0);
    rliteReply *reply;
    {
        int argc = 2;
        char *argv[] = {"get", "string"};
        size_t argvlen[] = {3, 6};
        reply = rliteCommandArgv(rl, argc, argv, argvlen);
        if (reply->type != RLITE_REPLY_STRING || reply->len != 5 || memcmp("value", reply->str, 5) != 0) {
            fprintf(stderr, "Unexpected reply on line %d\n", __LINE__);
            return 1;
        }
        rliteFreeReplyObject(reply);
    }
    {
        int argc = 4;
        char *argv[] = {"lrange", "list", "0", "-1"};
        size_t argvlen[] = {6, 4, 1, 2};
        reply = rliteCommandArgv(rl, argc, argv, argvlen);
        if (reply->type != RLITE_REPLY_ARRAY || reply->elements != 2) {
            fprintf(stderr, "Unexpected reply on line %d\n", __LINE__);
            return 1;
        }
        if (reply->element[0]->type != RLITE_REPLY_STRING || reply->element[0]->len != 1 || reply->element[0]->str[0] != '1') {
            fprintf(stderr, "Unexpected reply on line %d\n", __LINE__);
            return 1;
        }
        if (reply->element[1]->type != RLITE_REPLY_STRING || reply->element[1]->len != 1 || reply->element[1]->str[0] != '2') {
            fprintf(stderr, "Unexpected reply on line %d\n", __LINE__);
            return 1;
        }
        rliteFreeReplyObject(reply);
    }
    {
        int argc = 2;
        char *argv[] = {"smembers", "set"};
        size_t argvlen[] = {8, 3};
        reply = rliteCommandArgv(rl, argc, argv, argvlen);
        if (reply->type != RLITE_REPLY_ARRAY || reply->elements != 2) {
            fprintf(stderr, "Unexpected reply on line %d\n", __LINE__);
            return 1;
        }
        if (reply->element[0]->type != RLITE_REPLY_STRING || reply->element[0]->len != 1 || reply->element[0]->str[0] != '1') {
            fprintf(stderr, "Unexpected reply on line %d\n", __LINE__);
            return 1;
        }
        if (reply->element[1]->type != RLITE_REPLY_STRING || reply->element[1]->len != 1 || reply->element[1]->str[0] != '2') {
            fprintf(stderr, "Unexpected reply on line %d\n", __LINE__);
            return 1;
        }
        rliteFreeReplyObject(reply);
    }
    {
        int argc = 5;
        char *argv[] = {"zrange", "zset", "0", "-1", "WITHSCORES"};
        size_t argvlen[] = {6, 4, 1, 2, 10};
        reply = rliteCommandArgv(rl, argc, argv, argvlen);
        if (reply->type != RLITE_REPLY_ARRAY || reply->elements != 4) {
            fprintf(stderr, "Unexpected reply on line %d\n", __LINE__);
            return 1;
        }
        if (reply->element[0]->type != RLITE_REPLY_STRING || reply->element[0]->len != 1 || reply->element[0]->str[0] != '1') {
            fprintf(stderr, "Unexpected reply on line %d\n", __LINE__);
            return 1;
        }
        if (reply->element[1]->type != RLITE_REPLY_STRING || reply->element[1]->len != 1 || reply->element[1]->str[0] != '1') {
            fprintf(stderr, "Unexpected reply on line %d\n", __LINE__);
            return 1;
        }
        if (reply->element[2]->type != RLITE_REPLY_STRING || reply->element[2]->len != 1 || reply->element[2]->str[0] != '2') {
            fprintf(stderr, "Unexpected reply on line %d\n", __LINE__);
            return 1;
        }
        if (reply->element[3]->type != RLITE_REPLY_STRING || reply->element[3]->len != 1 || reply->element[3]->str[0] != '2') {
            fprintf(stderr, "Unexpected reply on line %d\n", __LINE__);
            return 1;
        }
        rliteFreeReplyObject(reply);
    }
    {
        int argc = 2;
        char *argv[] = {"HGETALL", "hash"};
        size_t argvlen[] = {7, 4};
        reply = rliteCommandArgv(rl, argc, argv, argvlen);
        if (reply->type != RLITE_REPLY_ARRAY || reply->elements != 4) {
            fprintf(stderr, "Unexpected reply on line %d\n", __LINE__);
            return 1;
        }
        if (reply->element[0]->type != RLITE_REPLY_STRING || reply->element[0]->len != 4 || memcmp(reply->element[0]->str, "key1", 4) != 0) {
            fprintf(stderr, "Unexpected reply on line %d\n", __LINE__);
            return 1;
        }
        if (reply->element[1]->type != RLITE_REPLY_STRING || reply->element[1]->len != 6 || memcmp(reply->element[1]->str, "value1", 6) != 0) {
            fprintf(stderr, "Unexpected reply on line %d\n", __LINE__);
            return 1;
        }
        if (reply->element[2]->type != RLITE_REPLY_STRING || reply->element[2]->len != 4 || memcmp(reply->element[2]->str, "key2", 4) != 0) {
            fprintf(stderr, "Unexpected reply on line %d\n", __LINE__);
            return 1;
        }
        if (reply->element[3]->type != RLITE_REPLY_STRING || reply->element[3]->len != 6 || memcmp(reply->element[3]->str, "value2", 6) != 0) {
            fprintf(stderr, "Unexpected reply on line %d\n", __LINE__);
            return 1;
        }
        rliteFreeReplyObject(reply);
    }
    return 0;
}
