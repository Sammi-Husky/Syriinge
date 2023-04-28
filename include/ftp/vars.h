#pragma once

#define BUFF_SIZE 1024
#define PATH_BUF_SIZE 4096

enum FTP_CMD {
    INVALID = -1,
    USER,
    PASS,
    RETR,
    STOR,
    STOU,
    APPE,
    REST,
    RNFR,
    RNTO,
    ABOR,
    DELE,
    RMD,
    MKD,
    PWD,
    CWD,
    CDUP,
    LIST,
    NLST,
    SITE,
    STAT,
    HELP,
    NOOP,
    TYPE,
    PASV,
    PORT,
    SYST,
    QUIT,
    MDTM,
    SIZE,
    FEAT,
    MLST,
    FTP_CMD_COUNT,
};

struct ftp_cmd {
    char* name;
    enum FTP_CMD cmd;
};

enum FTP_DATA_MODE {
    DATA_MODE_ASCII,
    DATA_MODE_IMAGE
};
enum FTP_XFER_MODE {
    XFER_MODE_PORT,
    XFER_MODE_PASV
};

extern struct ftp_cmd FTP_CMD_LIST[FTP_CMD_COUNT];