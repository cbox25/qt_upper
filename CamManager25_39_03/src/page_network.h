#ifndef PAGE_NETWORK_H
#define PAGE_NETWORK_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QString>
#include <QTextEdit>
#include "common.h"

#ifdef PRJ201

#define NETWORK_CMD_NUM_MAX 18
#define NETWORK_CMD_LEN_MAX 6
#define IMAGE_BOARD_NUM_MAX 1

typedef void (*NetworkBtnClickedEvent)(QObject *sender, int idx, int boardNum);

#ifdef _WIN32
#pragma pack(push, 1)

typedef struct {
    QString ip;
    int port;
} NetworkInfo;

typedef struct {
    uint8_t cnt;
    uint8_t cmd;
    uint16_t data1;
    uint16_t data2;
} CmdData;

typedef struct {
    uint16_t width;
    uint16_t height;
    uint8_t frameFreq;
} RecvInfo;

typedef struct {
    uint32_t armVersion;
    uint32_t fpgaVersion;
    uint16_t curDirect;
    uint8_t ddrInitStaus;
    uint32_t ddrInitTime; // 10ns
    RecvInfo dviInfo1;
    RecvInfo dviInfo2;
    uint8_t dviRecvDataErrCnt2;
    uint8_t dviRecvTimingErrCnt2;
    uint8_t dviRecvDataErrCnt1;
    uint8_t dviRecvTimingErrCnt1;
    RecvInfo sdiInfo1;
    RecvInfo sdiInfo2;
    uint8_t srioInitStatus;
    union {
        struct {
            uint32_t videoSize : 24;
            uint32_t videoType : 4;
            uint32_t reserve : 4;
        };
        uint32_t videoInfo;
    };

    union {
        struct {
            uint8_t isForwardVideoReq : 1;
            uint8_t isBackwardVideoReq : 1;
            uint8_t isPanoramaVideoReq : 1;
        };
        uint8_t videoReq;
    };

    uint32_t srioForwardVideoAddrMax;
    uint32_t srioBackwardVideoAddrMax;
    uint32_t srioPanoramaVideoAddrMax;

    union {
        struct {
            uint32_t srioSendToMainBoardBellFreq : 8;
            uint32_t srioSendToHelmetBellFreq : 8;
            uint32_t srioRecvVideoFrameFreqChn1 : 8;
            uint32_t srioRecvVideoFrameFreqChn0 : 8;
        };
        uint32_t srioInfo;
    };

    uint8_t gtxStatus;
    uint16_t preProcessHeartbeat;
    uint32_t preProcessVersion;
    uint16_t preProcessChnStatus;
    uint32_t guideDirect;
    uint8_t recognizeCtrl;
    uint8_t cmdCnt;
    uint8_t recvCmd[5];
    uint32_t distributedVersion1;
    uint8_t distributedVersion2to8[7];
} TestInfo;

#pragma pack(pop)
#else

typedef struct __attribute__((packed)) {
    QString ip;
    int port;
} NetworkInfo;

typedef struct __attribute__((packed)) {
    uint8_t cnt;
    uint8_t cmd;
    uint16_t data1;
    uint16_t data2;
} CmdData;

typedef struct __attribute__((packed)) {
    uint16_t width;
    uint16_t height;
    uint8_t frameFreq;
} RecvInfo;

typedef struct __attribute__((packed)) {
    uint32_t armVersion;
    uint32_t fpgaVersion;
    uint16_t curDirect;
    uint8_t ddrInitStaus;
    uint32_t ddrInitTime; // 10ns
    RecvInfo dviInfo1;
    RecvInfo dviInfo2;
    uint8_t dviRecvDataErrCnt2;
    uint8_t dviRecvTimingErrCnt2;
    uint8_t dviRecvDataErrCnt1;
    uint8_t dviRecvTimingErrCnt1;
    RecvInfo sdiInfo1;
    RecvInfo sdiInfo2;
    uint8_t srioInitStatus;
    union {
        struct {
            uint32_t videoSize : 24;
            uint32_t videoType : 4;
            uint32_t reserve : 4;
        };
        uint32_t videoInfo;
    };

    union {
        struct {
            uint8_t isForwardVideoReq : 1;
            uint8_t isBackwardVideoReq : 1;
            uint8_t isPanoramaVideoReq : 1;
        };
        uint8_t videoReq;
    };

    uint32_t srioForwardVideoAddrMax;
    uint32_t srioBackwardVideoAddrMax;
    uint32_t srioPanoramaVideoAddrMax;

    union {
        struct {
            uint32_t srioSendToMainBoardBellFreq : 8;
            uint32_t srioSendToHelmetBellFreq : 8;
            uint32_t srioRecvVideoFrameFreqChn1 : 8;
            uint32_t srioRecvVideoFrameFreqChn0 : 8;
        };
        uint32_t srioInfo;
    };

    uint8_t gtxStatus;
    uint16_t preProcessHeartbeat;
    uint32_t preProcessVersion;
    uint16_t preProcessChnStatus;
    uint32_t guideDirect;
    uint8_t recognizeCtrl;
    uint8_t cmdCnt;
    uint8_t recvCmd[5];
    uint32_t distributedVersion1;
    uint8_t distributedVersion2to8[7];
} TestInfo;

#endif

class TestInfoWidget {
public:
    QLabel *label;
    QLineEdit *edit;
};

class NetworkBtnInfo {
public:
    int idx;
    QPushButton *btn;
    QString name;
    NetworkBtnClickedEvent clickedEventCb;
};

void CreateNetworkWidgets(QWidget *pageNetwork, int boardNum);
/* cmd callback function */
void RollLeftBtnClickedEvent(QObject *sender, int idx, int boardNum);
void RollRightBtnClickedEvent(QObject *sender, int idx, int boardNum);
void RollSpeedUpBtnClickedEvent(QObject *sender, int idx, int boardNum);
void RollSpeedDownBtnClickedEvent(QObject *sender, int idx, int boardNum);
void SingleChnImageBtnClickedEvent(QObject *sender, int idx, int boardNum);
void TurnLeftCamBtnClickedEvent(QObject *sender, int idx, int boardNum);
void TurnRightCamBtnClickedEvent(QObject *sender, int idx, int boardNum);
void ShowPeriscopeVisibleBtnClickedEvent(QObject *sender, int idx, int boardNum);
void ShowPeriscopeIrBtnClickedEvent(QObject *sender, int idx, int boardNum);
void ShowSightVisibleBtnClickedEvent(QObject *sender, int idx, int boardNum);
void ShowSightIrBtnClickedEvent(QObject *sender, int idx, int boardNum);
void ShowMissileBtnClickedEvent(QObject *sender, int idx, int boardNum);
void TouchDisplayBtnClickedEvent(QObject *sender, int idx, int boardNum);
void ImageEnhanceBtnClickedEvent(QObject *sender, int idx, int boardNum);
void DirectGuideBtnClickedEvent(QObject *sender, int idx, int boardNum);
void CtrlRecognizeBtnClickedEvent(QObject *sender, int idx, int boardNum);
void GetVersionBtnClickedEvent(QObject *sender, int idx, int boardNum);
void GetTestInfoBtnClickedEvent(QObject *sender, int idx, int boardNum);
void SingleChnImageStop(int idx, int boardNum);

// 声明自适应布局函数
void UpdateTestInfoLayout(QWidget *pageNetwork, int zeroPointX, int zeroPointY, int w, int btnRowNum, int btnColNum);
void UdpSendDataWithIp(const char *buf, int len, QString srcIp, uint16_t srcPort, QString dstIp, uint16_t dstPort, QTextEdit *edit);
void UdpRecvDataWithIp(uint8_t *buf, int len, QString localIp, uint16_t localPort, QString remoteIp, uint16_t remotePort, QTextEdit *edit);
#endif // PRJ201

#endif // PAGE_NETWORK_H
