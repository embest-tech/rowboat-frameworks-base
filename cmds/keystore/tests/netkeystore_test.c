/*
 * Copyright (C) 2009 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common.h"
#include "keymgmt.h"

typedef int FUNC_PTR();
typedef struct {
    const char *name;
    FUNC_PTR *func;
} TESTFUNC;

#define FUNC_NAME(x) { #x, test_##x }
#define FUNC_BODY(x) int test_##x()

#define TEST_PASSWD        "12345678"
#define TEST_NPASSWD    "11111111"
#define TEST_DIR        "/data/local/tmp/keystore"
#define READONLY_DIR    "/proc/keystore"
#define TEST_NAMESPACE    "test"
#define TEST_KEYNAME    "key"
#define TEST_KEYNAME2    "key2"
#define TEST_KEYVALUE    "ANDROID"

void setup()
{
    if (init_keystore(TEST_DIR) != 0) {
        fprintf(stderr, "Can not create the test directory %s\n", TEST_DIR);
        exit(-1);
    }
}

void teardown()
{
    reset_keystore();
    rmdir(TEST_DIR);
}

FUNC_BODY(init_keystore)
{
    if (init_keystore(READONLY_DIR) == 0) return -1;

    return EXIT_SUCCESS;
}

FUNC_BODY(reset_keystore)
{
    chdir("/procx");
    if (reset_keystore() == 0) return -1;
    chdir(TEST_DIR);
    return EXIT_SUCCESS;
}

FUNC_BODY(get_state)
{
    if (get_state() != UNINITIALIZED) return -1;
    passwd(TEST_PASSWD);
    if (get_state() != UNLOCKED) return -1;
    lock();
    if (get_state() != LOCKED) return -1;
    reset_keystore();
    if (get_state() != UNINITIALIZED) return -1;
    return EXIT_SUCCESS;
}

FUNC_BODY(passwd)
{
    char buf[512];

    if (passwd(" 23432dsfsdf") == 0) return -1;
    if (passwd("dsfsdf") == 0) return -1;
    passwd(TEST_PASSWD);
    lock();
    if (unlock("55555555") == 0) return -1;
    if (unlock(TEST_PASSWD) != 0) return -1;

    // change the password
    sprintf(buf, "%s %s", "klfdjdsklfjg", "abcdefghi");
    if (passwd(buf) == 0) return -1;

    sprintf(buf, "%s %s", TEST_PASSWD, TEST_NPASSWD);
    if (passwd(buf) != 0) return -1;
    lock();

    if (unlock(TEST_PASSWD) == 0) return -1;
    if (unlock(TEST_NPASSWD) != 0) return -1;

    return EXIT_SUCCESS;
}

FUNC_BODY(lock)
{
    if (lock() == 0) return -1;
    passwd(TEST_PASSWD);
    if (lock() != 0) return -1;
    if (lock() != 0) return -1;
    return EXIT_SUCCESS;
}

FUNC_BODY(unlock)
{
    int i = MAX_RETRY_COUNT;
    passwd(TEST_PASSWD);
    lock();
    while (i > 1) {
        if (unlock(TEST_NPASSWD) != --i) return -1;
    }
    if (unlock(TEST_NPASSWD) != -1) return -1;
    return EXIT_SUCCESS;
}

FUNC_BODY(put_key)
{
    int i = 0;
    char keyname[512];

    if (put_key(TEST_NAMESPACE, TEST_KEYNAME, (unsigned char *)TEST_KEYVALUE,
                strlen(TEST_KEYVALUE)) == 0) return -1;
    passwd(TEST_PASSWD);
    if (put_key(TEST_NAMESPACE, TEST_KEYNAME, (unsigned char *)TEST_KEYVALUE,
                strlen(TEST_KEYVALUE)) != 0) return -1;

    for(i = 0; i < 500; i++) keyname[i] = 'K';
    keyname[i] = 0;
    if (put_key(TEST_NAMESPACE, keyname, (unsigned char *)TEST_KEYVALUE,
                strlen(TEST_KEYVALUE)) == 0) return -1;
    if (put_key(TEST_NAMESPACE, TEST_KEYNAME, (unsigned char *)TEST_KEYVALUE,
                MAX_KEY_VALUE_LENGTH + 1) == 0) return -1;
    return EXIT_SUCCESS;
}

FUNC_BODY(get_key)
{
    int size;
    unsigned char data[MAX_KEY_VALUE_LENGTH];

    if (get_key(TEST_NAMESPACE, TEST_KEYNAME, data, &size) == 0) return -1;

    passwd(TEST_PASSWD);
    put_key(TEST_NAMESPACE, TEST_KEYNAME, (unsigned char *)TEST_KEYVALUE,
            strlen(TEST_KEYVALUE));
    if (get_key(TEST_NAMESPACE, TEST_KEYNAME, data, &size) != 0) return -1;
    if (memcmp(data, TEST_KEYVALUE, size) != 0) return -1;

    return EXIT_SUCCESS;
}

FUNC_BODY(remove_key)
{
    if (remove_key(TEST_NAMESPACE, TEST_KEYNAME) == 0) return -1;

    passwd(TEST_PASSWD);
    if (remove_key(TEST_NAMESPACE, TEST_KEYNAME) == 0) return -1;

    put_key(TEST_NAMESPACE, TEST_KEYNAME, (unsigned char *)TEST_KEYVALUE,
            strlen(TEST_KEYVALUE));
    if (remove_key(TEST_NAMESPACE, TEST_KEYNAME) != 0) return -1;

    return EXIT_SUCCESS;
}

FUNC_BODY(list_keys)
{
    int i;
    char buf[128];
    char reply[BUFFER_MAX];

    for(i = 0; i < 100; i++) buf[i] = 'K';
    buf[i] = 0;

    if (list_keys(TEST_NAMESPACE, reply) == 0) return -1;

    passwd(TEST_PASSWD);
    if (list_keys(buf, reply) == 0) return -1;

    if (list_keys(TEST_NAMESPACE, reply) != 0) return -1;
    if (strcmp(reply, "") != 0) return -1;

    put_key(TEST_NAMESPACE, TEST_KEYNAME, (unsigned char *)TEST_KEYVALUE,
            strlen(TEST_KEYVALUE));
    if (list_keys(TEST_NAMESPACE, reply) != 0) return -1;
    if (strcmp(reply, TEST_KEYNAME) != 0) return -1;

    put_key(TEST_NAMESPACE, TEST_KEYNAME2, (unsigned char *)TEST_KEYVALUE,
            strlen(TEST_KEYVALUE));

    if (list_keys(TEST_NAMESPACE, reply) != 0) return -1;
    sprintf(buf, "%s %s", TEST_KEYNAME2, TEST_KEYNAME);
    if (strcmp(reply, buf) != 0) return -1;

    return EXIT_SUCCESS;
}

TESTFUNC all_tests[] = {
    FUNC_NAME(init_keystore),
    FUNC_NAME(reset_keystore),
    FUNC_NAME(get_state),
    FUNC_NAME(passwd),
    FUNC_NAME(lock),
    FUNC_NAME(unlock),
    FUNC_NAME(put_key),
    FUNC_NAME(get_key),
    FUNC_NAME(remove_key),
    FUNC_NAME(list_keys),
};

int main(int argc, char **argv) {
    int i, ret;
    for (i = 0 ; i < (int)(sizeof(all_tests)/sizeof(TESTFUNC)) ; ++i) {
        setup();
        if ((ret = all_tests[i].func()) != EXIT_SUCCESS) {
            fprintf(stderr, "ERROR in function %s\n", all_tests[i].name);
            return ret;
        } else {
            fprintf(stderr, "function %s PASSED!\n", all_tests[i].name);
        }
        teardown();
    }
    return EXIT_SUCCESS;
}
