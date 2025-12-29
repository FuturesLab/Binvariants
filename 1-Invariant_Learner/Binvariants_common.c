#include "Binvariants_common.h"

int binvTB_struct_shm_id;
BinvariantsTB *binvTB;

int binv_id_shm_id;
uint16_t *binv_id;

int get_text_range_gdb(const char *binary, uint64_t *start, uint64_t *end, uint64_t *init) {
    char command[512];
    FILE *fp;
    char *output = NULL;
    size_t output_size = 0;
    char buffer[1024];

    snprintf(command, sizeof(command),
             "gdb -batch -ex \"file %s\" -ex \"info files\"", binary);

    fp = popen(command, "r");
    if (!fp) {
        perror("popen failed");
        return -1;
    }

    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        size_t len = strlen(buffer);
        char *new_output = realloc(output, output_size + len + 1);
        if (!new_output) {
            free(output);
            perror("realloc failed");
            pclose(fp);
            return -1;
        }
        output = new_output;
        memcpy(output + output_size, buffer, len);
        output_size += len;
        output[output_size] = '\0';
    }
    pclose(fp);

    regex_t regex;
    regmatch_t matches[3];
    int ret;

    ret = regcomp(&regex, "0x([0-9a-fA-F]+) - 0x([0-9a-fA-F]+) is \\.text", REG_EXTENDED);
    if (ret) {
        fprintf(stderr, "Could not compile regex for .text\n");
        free(output);
        return -1;
    }

    ret = regexec(&regex, output, 3, matches, 0);
    if (!ret) {
        int start_len = matches[1].rm_eo - matches[1].rm_so;
        char start_str[32];
        if (start_len >= (int)sizeof(start_str))
            start_len = sizeof(start_str) - 1;
        strncpy(start_str, output + matches[1].rm_so, start_len);
        start_str[start_len] = '\0';
        *start = strtoul(start_str, NULL, 16);

        int end_len = matches[2].rm_eo - matches[2].rm_so;
        char end_str[32];
        if (end_len >= (int)sizeof(end_str))
            end_len = sizeof(end_str) - 1;
        strncpy(end_str, output + matches[2].rm_so, end_len);
        end_str[end_len] = '\0';
        *end = strtoul(end_str, NULL, 16);
    } else {
        fprintf(stderr, "No .text match found in GDB output\n");
        regfree(&regex);
        free(output);
        return -1;
    }
    regfree(&regex);

    ret = regcomp(&regex, "0x([0-9a-fA-F]+) - 0x([0-9a-fA-F]+) is \\.init", REG_EXTENDED);
    if (ret) {
        fprintf(stderr, "Could not compile regex for .init\n");
        free(output);
        return -1;
    }

    ret = regexec(&regex, output, 3, matches, 0);
    if (!ret) {
        int init_len = matches[1].rm_eo - matches[1].rm_so;
        char init_str[32];
        if (init_len >= (int)sizeof(init_str))
            init_len = sizeof(init_str) - 1;
        strncpy(init_str, output + matches[1].rm_so, init_len);
        init_str[init_len] = '\0';
        *init = strtoul(init_str, NULL, 16);
    } else {
        fprintf(stderr, "No .init match found in GDB output\n");
        regfree(&regex);
        free(output);
        return -1;
    }
    regfree(&regex);
    free(output);
    return 0;
}

void init_binvTB_struct(void) {
    binvTB_struct_shm_id = shmget(IPC_PRIVATE, BinvTBSize * sizeof(BinvariantsTB), 
            IPC_CREAT | IPC_EXCL | 0600);
    if (binvTB_struct_shm_id < 0) { 
        perror("shmget failed");
        exit(1);
    }

    printf("binvTB_struct_shm_id: %d\n", binvTB_struct_shm_id);

    SET_INT_ENV(binvTB_struct_shm_id, BINVTB_SHM_ID_ENV);
}


void init_binvTB(void) {
    init_binvTB_struct();
    binvTB = (BinvariantsTB *)shmat(binvTB_struct_shm_id, NULL, 0);

    if (binvTB == (void *)-1) {
        perror("Failed to attach binvTB\n");
        exit(0);
    }
    
    memset(binvTB, 0, BinvTBSize * sizeof(BinvariantsTB));
}

void attach_binvTB(void) {
    char *id_str = getenv(BINVTB_SHM_ID_ENV);

    if (!id_str) {
        perror("Fail to get BINVTB_SHM_ID_ENV\n");
        exit(0);
    }

    binvTB_struct_shm_id = atoi(id_str);

    binvTB = (BinvariantsTB *)shmat(binvTB_struct_shm_id, NULL, 0);
    if (binvTB == (void *)-1) {
        perror("Failed to attach binvTB\n");
        exit(0);
    }
}

void deinit_binvTB(void) {
    shmctl(binvTB_struct_shm_id, IPC_RMID, NULL);
    unsetenv(BINVTB_SHM_ID_ENV);
}

void init_binv_id_shm(void) {
    binv_id_shm_id = shmget(IPC_PRIVATE, sizeof(uint16_t), IPC_CREAT | 0666);
    if (binv_id_shm_id < 0) { 
        perror("shmget failed");
        exit(1);
    }

    printf("binv_id_shm_id: %d\n", binv_id_shm_id);

    SET_INT_ENV(binv_id_shm_id, BINV_ID_SHM_ID_ENV);
}

void init_binv_id_ptr(void) {
    init_binv_id_shm();

    binv_id = (uint16_t *) shmat(binv_id_shm_id, NULL, 0);

    if (binv_id == (uint16_t *) -1) {
        perror("Failed to attach binv_id_ptr");
        exit(1);
    }

    *binv_id = 1;
}

void attach_binv_id_ptr(void) {
    char *id_str = getenv(BINV_ID_SHM_ID_ENV);

    if (!id_str) {
        perror("Fail to get BINV_ID_SHM_ID_ENV\n");
        exit(0);
    }

    binv_id_shm_id = atoi(id_str);

    binv_id = (uint16_t *) shmat(binv_id_shm_id, NULL, 0);
    if (binv_id == (uint16_t *) -1) {
        perror("Failed to attach binv_id_ptr");
        exit(0);
    }

    *binv_id = 1;
}

void deinit_binv_id_ptr(void) {
    shmctl(binv_id_shm_id, IPC_RMID, NULL);
    unsetenv(BINV_ID_SHM_ID_ENV);
}