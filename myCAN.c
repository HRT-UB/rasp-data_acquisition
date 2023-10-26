#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "controlcan.h"
#include <time.h>
#include <sys/time.h>
#include <sqlite3.h>
#define msleep(ms) usleep((ms) * 1000)
#define min(a, b) (((a) < (b)) ? (a) : (b))
#define RX_BUFF_SIZE (1000)
#define RX_WAIT_TIME (100)

unsigned gDevType = 3;   // usbcan-i
unsigned gDevIdx = 0;    // Card0
unsigned gChMask = 1;    // CAN1
unsigned gBaud = 0x1C00; // 500K

struct UTC_TIME
{
    int year;
    int month;
    int day;
    int hour;
    int sec;
    int min;
    int msec;
};

struct OIL_MSG
{
    float lf_oil;
    float rf_oil;
    float lb_oil;
    float rb_oil;
};

struct AIR_MSG
{
    float f_air;
    float b_air;
};
struct SPEED_MSG
{
    float c_speed;
    float e_speed;
};
sqlite3 *db;
int8_t air_cnt = 0;
int8_t oil_cnt = 0;

void start_receive(void);            // 开启接收CAN frame
void rx_process(unsigned);           // 接收CAN
int verify_frame(VCI_CAN_OBJ *can);  // 对帧数据进行验证
void frame_output(VCI_CAN_OBJ *can); // 输出CAN数据
int db_init(void);
void getTime(char *ret);
int data_process(VCI_CAN_OBJ caninfo, struct OIL_MSG *oil, struct AIR_MSG *air, struct SPEED_MSG *speed);
int insert_msg(char *date, struct OIL_MSG oil, struct AIR_MSG air, struct SPEED_MSG e_speed);
int create_table();
int clear_msg();
int main(int argc, char *argv[])
{
    if (argc > 2)
    {
        printf("myCAN [param]\n"
               "example: myCAN db_create\n"
               "                   |\n"
               "                   |\n"
               "                   db_create, db_delete, receive_pressure\n");
        return 0;
    }
    if (argc == 1 || strcmp(argv[1], "receive_pressure") == 0)
    {
        printf("DevType: usbcan-i\nDevIdx: Card0\nChMask: CAN1\nBaud: 500K\n");
        if (db_init() == -1)
        {
            return 0;
        }

        if (!VCI_OpenDevice(gDevType, gDevIdx, 0))
        {
            printf("VCI_OpenDevice failed\n");
            goto DB_close;
        }
        printf("VCI_OpenDevice succeeded\n");

        start_receive();

        VCI_CloseDevice(gDevType, gDevIdx);
        printf("VCI_CloseDevice\n");

    DB_close:
        sqlite3_close(db);
        printf("Datebase close\n");
        return 0;
    }
    else if (strcmp(argv[1], "db_create") == 0)
    {
        create_table();
    }
    else if (strcmp(argv[1], "db_delete") == 0)
    {
        clear_msg();
    }
    else
    {
        printf("myCAN [param]\n"
               "example: myCAN db_create\n"
               "                   |\n"
               "                   |\n"
               "                   db_create, db_delete, receive_pressure\n");
        return 0;
    }
}

// 创建接受线程
void start_receive(void)
{
    // -----init & start------

    VCI_INIT_CONFIG config;
    // config.AccCode = 0x402; // 接收0x402
    config.AccCode = 0x00; 
    config.AccMask = 0xffffffff;
    config.Filter = 1;
    config.Mode = 0;
    config.Timing0 = gBaud & 0xff;
    config.Timing1 = (gBaud >> 8) & 0xff;
    unsigned canid = 0;
    for (int i = 0; i < 4; ++i) // 在本程序中只支持开放一个CAN
        canid = gChMask & (1 << i);
    if (!VCI_InitCAN(gDevType, gDevIdx, canid, &config))
    {
        printf("VCI_InitCAN(%d) failed\n", canid);
        return;
    }
    printf("VCI_InitCAN(%d) succeeded\n", canid);

    if (!VCI_StartCAN(gDevType, gDevIdx, canid))
    {
        printf("VCI_StartCAN(%d) failed\n", canid);
        return;
    }

    // printf("VCI_StartCAN(%d) succeeded\n", canid);

    // ------can receive create------

    rx_process(canid);
}

void rx_process(unsigned canid)
{

    VCI_CAN_OBJ can_buffer[RX_BUFF_SIZE]; // can接收缓存区
    int cnt;
    int i;
    unsigned check_point = 0;
    int err = 0;
    char date[35];
    struct OIL_MSG oil;
    struct AIR_MSG air;
    struct SPEED_MSG speed;
    while (!err)
    {
        cnt = VCI_Receive(gDevType, gDevIdx, canid, can_buffer, RX_BUFF_SIZE, RX_WAIT_TIME);
        if (!cnt)
            continue;
        for (i = 0; i < cnt; ++i)
        {
            if (verify_frame(&can_buffer[i]))
            {
                // frame_output(&can_buffer[i]);

                getTime(date);
                int ret = data_process(can_buffer[i], &oil, &air, &speed);
                // printf("canid: %x\n", can_buffer[i].ID);
                if (ret == -1)
                {
                    err = 1;
                    break;
                }
                else if (ret == 1)
                {
                    oil_cnt = 0;
                    air_cnt = 0;

                    if (insert_msg(date, oil, air, speed) == -1)
                    {
                        err = 1;
                        break;
                    }
                    printf("insert_msg\n");
                }
                continue;
            }

            printf("CAN%d: verify_frame() failed\n", canid);
            err = 1;
        }
    }
}

int data_process(VCI_CAN_OBJ caninfo, struct OIL_MSG *oil, struct AIR_MSG *air, struct SPEED_MSG *speed)
{
    int recv = 0;
    if (caninfo.ID == 0x20) printf("receive 0x20\r\n");
    switch (caninfo.ID)
    {
    case 0x482: // 气压
        if (caninfo.DataLen != 4)
        {
            printf("CAN 0x%x datalen error, len:%d\n", caninfo.ID, caninfo.DataLen);
            return -1; // 错误
        }

        air->f_air = (float)(caninfo.Data[0] | (caninfo.Data[1] << 8)) / 100;
        air->b_air = (float)(caninfo.Data[2] | (caninfo.Data[3] << 8)) / 100;

        // if (air_cnt < 3)
        // { // 每四次存一次数据库，那么就是0.2s存一个
        //     air_cnt++;
        //     return 0; // 不做处理
        // }

        // else
        // {
        //     return 1; // 进行处理
        // }
        return 1;
        break;
    case 0x402: // 油压
        if (caninfo.DataLen != 8)
        {
            printf("CAN 0x%x datalen error, len:%d\n", caninfo.ID, caninfo.DataLen);
            return -1; // 错误
        }

        oil->lf_oil = (float)(caninfo.Data[0] | (caninfo.Data[1] << 8)) / 100;
        oil->rf_oil = (float)(caninfo.Data[2] | (caninfo.Data[3] << 8)) / 100;
        oil->lb_oil = (float)(caninfo.Data[4] | (caninfo.Data[5] << 8)) / 100;
        oil->rb_oil = (float)(caninfo.Data[6] | (caninfo.Data[7] << 8)) / 100;

        // if (oil_cnt < 3)
        // { // 每四次存一次数据库，那么就是0.2s存一个
        //     oil_cnt++;
        //     return 0; // 不做处理
        // }

        // else
        // {
        //     return 1; // 进行处理
        // }
        return 1;
        break;
    case 0x20:
        // printf("%d %d %d %d\r\n", caninfo.Data[1], caninfo.Data[2], caninfo.Data[5], caninfo.Data[6]);
        speed->e_speed = (float)caninfo.Data[1] + (float)caninfo.Data[2] / 100;
        speed->c_speed = (float)caninfo.Data[5] + (float)caninfo.Data[6] / 100;
        break;
    default:
        break;
    }
    return 0;
}
void frame_output(VCI_CAN_OBJ *can)
{
    printf("TimeStamp: %d, ID: 0x%x Len: %d ", can->TimeStamp, can->ID, can->DataLen);
    for (int i = 0; i < can->DataLen; ++i)
    {
        printf("%x ", can->Data[i]);
    }
    printf("\n");
}

int verify_frame(VCI_CAN_OBJ *can)
{
    if (can->DataLen > 8)
        return 0; // error: data length
    return 1;     // ext-frame ok
}

void getTime(char *ret)
{
    struct timeval tv;
    struct tm ts;
    char buf[22];
    time_t now;
    gettimeofday(&tv, NULL);

    // utc->msec = tv.tv_usec / 1000;
    now = time(&tv.tv_sec);
    ts = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &ts);
    sprintf(ret, "%s.%06ld", buf, tv.tv_usec);
}
int db_init(void)
{
    sqlite3 *db;
    int rc = 0;
    int cnt = 100;
    while (cnt--)
    {
        rc = sqlite3_open("EBS.db", &db);

        if (rc)
        {
            printf("Can't open datebase: %s\n", sqlite3_errmsg(db));
            if (cnt == 0)
                return -1;
            else
                continue;
        }
    }

    printf("Opened datebase successfully\n");
    sqlite3_close(db);

    return 0;
}

int create_table()
{
    char *sql;
    int rc;
    char *zErrMsg = 0;
    sql = "CREATE TABLE PRESSURE("
          "DATE      CHAR(50) PRIMARY KEY NOT NULL,"
          "LF_OIL    REAL      NOT NULL,"
          "RF_OIL    REAL      NOT NULL,"
          "LB_OIL    REAL      NOT NULL,"
          "RB_OIL    REAL      NOT NULL,"
          "F_AIR     REAL      NOT NULL,"
          "B_AIR     REAL      NOT NULL,"
          "E_SPEED   REAL      NOT NULL,"
          "C_SPEED   REAL      NOT NULL)";

    rc = sqlite3_open("EBS.db", &db);
    if (rc)
    {
        printf("Can't open datebase: %s\n", sqlite3_errmsg(db));
        return -1;
    }
    rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        printf("SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        return -1;
    }
    printf("Table created successfully\n");
    sqlite3_close(db);
    return 0;
}

int insert_msg(char *date, struct OIL_MSG oil, struct AIR_MSG air, struct SPEED_MSG speed)
{
    char *zErrMsg = 0;
    int rc = 0;
    char sql[200];
    char value[100];
    int cnt = 100;
    while (cnt--)
    {
        rc = sqlite3_open("EBS.db", &db);
        if (rc)
        {
            printf("Can't open datebase: %s\n", sqlite3_errmsg(db));
            if (cnt == 0)
                return -1;
            else
                continue;
        }
        sprintf(value, "VALUES ('%s', %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f);", date, oil.lf_oil, oil.rf_oil, oil.lb_oil, oil.rb_oil, air.f_air, air.b_air, speed.e_speed, speed.c_speed);
        sprintf(sql, "INSERT INTO PRESSURE (DATE, LF_OIL, RF_OIL, LB_OIL, RB_OIL, F_AIR, B_AIR, E_SPEED, C_SPEED) %s", value);

        rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);

        if (rc != SQLITE_OK)
        {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);

            if (cnt == 0)
            {
                sqlite3_free(zErrMsg);
                return -1;
            }

            else
            {
                sqlite3_close(db);
                continue;
            }
        }
        sqlite3_close(db);
        break;
    }

    return 0;
}

int clear_msg()
{
    char *zErrMsg = 0;
    int rc = 0;
    char *sql;
    char value[100];
    int cnt = 100;
    while (cnt--)
    {
        rc = sqlite3_open("EBS.db", &db);
        if (rc)
        {
            printf("Can't open datebase: %s\n", sqlite3_errmsg(db));
            if (cnt == 0)
                return -1;
            else
                continue;
        }
    }
    sql = "DELETE from PRESSURE";
    rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(db);
        return -1;
    }
    printf("delete successful\n");
    sqlite3_close(db);
    return 0;
}