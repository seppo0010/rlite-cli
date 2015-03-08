/*
 * Copyright (c) 2009-2012, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "hirlite.h"
#include "sds.h"
#include "lzf.h"    /* LZF compression library */

#define REDIS_RDB_LENERR UINT_MAX

typedef FILE rio;

static int rioRead(rio *rdb, void *t, size_t len) {
    if (fread(t, sizeof(char), len, rdb) != len) {
        return 0;
    }
    return 1;
}

/* Load an encoded length. The "isencoded" argument is set to 1 if the length
 * is not actually a length but an "encoding type". See the REDIS_RDB_ENC_*
 * definitions in rdb.h for more information. */
static uint32_t rdbLoadLen(rio *rdb, int *isencoded) {
    unsigned char buf[2];
    uint32_t len;
    int type;

    if (isencoded) *isencoded = 0;
    if (rioRead(rdb,buf,1) == 0) return REDIS_RDB_LENERR;
    type = (buf[0]&0xC0)>>6;
    if (type == REDIS_RDB_ENCVAL) {
        /* Read a 6 bit encoding type. */
        if (isencoded) *isencoded = 1;
        return buf[0]&0x3F;
    } else if (type == REDIS_RDB_6BITLEN) {
        /* Read a 6 bit len. */
        return buf[0]&0x3F;
    } else if (type == REDIS_RDB_14BITLEN) {
        /* Read a 14 bit len. */
        if (rioRead(rdb,buf+1,1) == 0) return REDIS_RDB_LENERR;
        return ((buf[0]&0x3F)<<8)|buf[1];
    } else {
        /* Read a 32 bit len. */
        if (rioRead(rdb,&len,4) == 0) return REDIS_RDB_LENERR;
        return ntohl(len);
    }
}

static sds rdbLoadLzfStringObject(rio *rdb) {
    unsigned int len, clen;
    unsigned char *c = NULL;
    sds val = NULL;

    if ((clen = rdbLoadLen(rdb,NULL)) == REDIS_RDB_LENERR) return NULL;
    if ((len = rdbLoadLen(rdb,NULL)) == REDIS_RDB_LENERR) return NULL;
    if ((c = malloc(clen)) == NULL) goto err;
    if ((val = sdsnewlen(NULL,len)) == NULL) goto err;
    if (rioRead(rdb,c,clen) == 0) goto err;
    if (rl_lzf_decompress(c,clen,val,len) == 0) goto err;
    free(c);
    return val;
err:
    free(c);
    sdsfree(val);
    return NULL;
}
/* Loads an integer-encoded object with the specified encoding type "enctype".
 * If the "encode" argument is set the function may return an integer-encoded
 * string object, otherwise it always returns a raw string object. */
static sds rdbLoadIntegerObject(rio *rdb, int enctype, int encode) {
    unsigned char enc[4];
    long long val;

    if (enctype == REDIS_RDB_ENC_INT8) {
        if (rioRead(rdb,enc,1) == 0) return NULL;
        val = (signed char)enc[0];
    } else if (enctype == REDIS_RDB_ENC_INT16) {
        uint16_t v;
        if (rioRead(rdb,enc,2) == 0) return NULL;
        v = enc[0]|(enc[1]<<8);
        val = (int16_t)v;
    } else if (enctype == REDIS_RDB_ENC_INT32) {
        uint32_t v;
        if (rioRead(rdb,enc,4) == 0) return NULL;
        v = enc[0]|(enc[1]<<8)|(enc[2]<<16)|(enc[3]<<24);
        val = (int32_t)v;
    } else {
        val = 0; /* anti-warning */
        fprintf(stderr, "Unknown RDB integer encoding type\n");
        exit(1);
    }
    return sdsfromlonglong(val);
}

static sds rdbGenericLoadStringObject(rio *rdb, int encode) {
    int isencoded;
    uint32_t len;
    sds o;

    len = rdbLoadLen(rdb,&isencoded);
    if (isencoded) {
        switch(len) {
        case REDIS_RDB_ENC_INT8:
        case REDIS_RDB_ENC_INT16:
        case REDIS_RDB_ENC_INT32:
            return rdbLoadIntegerObject(rdb,len,encode);
        case REDIS_RDB_ENC_LZF:
            return rdbLoadLzfStringObject(rdb);
        default:
            fprintf(stderr, "Unknown RDB encoding type\n");
            return NULL;
        }
    }

    if (len == REDIS_RDB_LENERR) return NULL;
    o = sdsgrowzero(sdsempty(), len);
    if (len && rioRead(rdb,o,len) == 0) {
        sdsfree(o);
        return NULL;
    }
    return o;
}

sds rdbLoadStringObject(rio *rdb) {
    return rdbGenericLoadStringObject(rdb,0);
}

static int rdbLoadType(rio *rdb) {
    unsigned char type;
    if (rioRead(rdb,&type,1) == 0) return -1;
    return type;
}

static time_t rdbLoadTime(FILE *rdb) {
    int32_t t32;
    if (rioRead(rdb,&t32,4) == 0) return -1;
    return (time_t)t32;
}

static long long rdbLoadMillisecondTime(rio *rdb) {
    int64_t t64;
    if (rioRead(rdb,&t64,8) == 0) return -1;
    return (long long)t64;
}

struct rl_rio_streamer {
    rio *rdb;
    int *type;
};

static int rl_to_rio_read(struct rl_restore_streamer *streamer, unsigned char *str, long len) {
    struct rl_rio_streamer *rio_streamer = streamer->context;
    if (rio_streamer->type) {
        if (len == 1) {
            int type = *rio_streamer->type;
            rio_streamer->type = NULL;
            *str = type;
            return RL_OK;
        } else {
            return RL_UNEXPECTED;
        }
    }
    return rioRead(rio_streamer->rdb, str, len) == 0 ? RL_UNEXPECTED : RL_OK;
}

int rdb2rld(const char *source, const char *target) {
    FILE* rdb = fopen(source, "r");
    if (!rdb) {
        fprintf(stderr, "File not found: %s\n", source);
        return 1;
    }
    FILE* rlTest = fopen(target, "r");
    if (rlTest) {
        fclose(rlTest);
        fprintf(stderr, "Target file must not exist\n");
        return 1;
    }
    rliteContext *rl = rliteConnect(target, 0);
    char buffer[10];
    int type, retval;
    long long expiretime, now = rl_mstime();
    uint32_t dbid;

    if (fread(buffer, sizeof(char), 9, rdb) != 9) {
        fprintf(stderr, "Source is not a valid rdb file\n");
        return 1;
    }

    if (memcmp(buffer, "REDIS0006", 9) != 0) {
        fprintf(stderr, "Source is not a supported rdb file\n");
        return 1;
    }

    sds key;

    rl_restore_streamer rlite_streamer;
    struct rl_rio_streamer rio_streamer;
    rio_streamer.rdb = rdb;
    rlite_streamer.context = &rio_streamer;
    rlite_streamer.read = rl_to_rio_read;
    while(1) {
        expiretime = -1;

        /* Read type. */
        if ((type = rdbLoadType(rdb)) == -1) goto eoferr;
        if (type == REDIS_RDB_OPCODE_EXPIRETIME) {
            if ((expiretime = rdbLoadTime(rdb)) == -1) goto eoferr;
            /* We read the time so we need to read the object type again. */
            if ((type = rdbLoadType(rdb)) == -1) goto eoferr;
            /* the EXPIRETIME opcode specifies time in seconds, so convert
             * into milliseconds. */
            expiretime *= 1000;
        } else if (type == REDIS_RDB_OPCODE_EXPIRETIME_MS) {
            /* Milliseconds precision expire times introduced with RDB
             * version 3. */
            if ((expiretime = rdbLoadMillisecondTime(rdb)) == -1) goto eoferr;
            /* We read the time so we need to read the object type again. */
            if ((type = rdbLoadType(rdb)) == -1) goto eoferr;
        }

        if (type == REDIS_RDB_OPCODE_EOF)
            break;

        /* Handle SELECT DB opcode as a special case */
        if (type == REDIS_RDB_OPCODE_SELECTDB) {
            if ((dbid = rdbLoadLen(rdb,NULL)) == REDIS_RDB_LENERR)
                goto eoferr;
            if (dbid >= (unsigned)rl->db->number_of_databases) {
                fprintf(stderr, "FATAL: Data file was created with a Redis server configured to handle more than %d databases. Exiting\n", rl->db->number_of_databases);
                exit(1);
            }
            rl->db->selected_database = dbid;
            continue;
        }
        /* Read key */
        if ((key = rdbLoadStringObject(rdb)) == NULL) goto eoferr;
        rio_streamer.type = &type;

        /**
         * Check if the key already expired.
         **/
        if (expiretime != -1 && expiretime < now) {
            retval = rl_restore_stream(rl->db, NULL, 0, 0, &rlite_streamer);
        } else {
            retval = rl_restore_stream(rl->db, (unsigned char *)key, sdslen(key), 0, &rlite_streamer);
        }
        sdsfree(key);
    }
    rl_commit(rl->db);

    rliteFree(rl);
    fclose(rdb);
    return 0;
eoferr:
    fprintf(stderr, "Early eof\n");
    rliteFree(rl);
    fclose(rdb);
    return 1;
}
