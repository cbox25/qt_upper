#ifndef MULTI_THREADS_H
#define MULTI_THREADS_H

#include "qwaitcondition.h"
#include <QCoreApplication>
#include <QThreadPool>
#include <QRunnable>
#include <QDebug>
#include <QThread>
#include <QQueue>
#include <QByteArray>
#include <QMutex>
#include <QEvent>
#include <QObject>
#include <QMap>

typedef void (*ThreadCb)(void*); // match UpdatePageFw
typedef void (*CmdCb)(void *param);

#define THREAD_PACKET_HEAD    ((uint16_t) 0xEB90)
#define THREAD_PACKET_DATA_LEN  512

// type cmd
enum {
    THREAD_TYPE_CMD_FIRST = 0x00,
    THREAD_TYPE_CMD_REG_CTRL = 0x01,
    THREAD_TYPE_CMD_UPDATE_FW = 0x02,
    THREAD_TYPE_CMD_USB3 = 0x03,
    THREAD_TYPE_CMD_LOGGER = 0x04, // New log type added
    THREAD_TYPE_CMD_NUM
};

// type ack
enum {
    THREAD_TYPE_ACK_FIRST = 0x80,
    THREAD_TYPE_ACK_REG_CTRL = 0x81,
    THREAD_TYPE_ACK_UPDATE_FW = 0x82,
    THREAD_TYPE_ACK_USB3 = 0x83,
    THREAD_TYPE_ACK_LOGGER = 0x84, // New log confirmation types have been added
    THREAD_TYPE_ACK_LAST
};
#define THREAD_TYPE_ACK_IDX(ack) (ack - THREAD_TYPE_ACK_FIRST)
#define THREAD_TYPE_ACK_NUM (THREAD_TYPE_ACK_LAST - THREAD_TYPE_ACK_FIRST)

// update subType cmd
enum {
    THREAD_SUBTYPE_UPDATE_CMD_FIRST = 0x00,
    THREAD_SUBTYPE_UPDATE_CMD_UPDATE_FW = 0x01,
    THREAD_SUBTYPE_UPDATE_CMD_LAST
};

// update subType ack
enum {
    THREAD_SUBTYPE_UPDATE_ACK_FIRST = 0x00,
    THREAD_SUBTYPE_UPDATE_ACK_PROGRESS_BAR = 0x01,
    THREAD_SUBTYPE_UPDATE_ACK_RET = 0x02,
    THREAD_SUBTYPE_UPDATE_ACK_LAST
};

// usb subType cmd
enum {
    THREAD_SUBTYPE_USB_CMD_FIRST = 0x00,
    THREAD_SUBTYPE_USB_CMD_START = 0x01,
    THREAD_SUBTYPE_USB_CMD_STOP = 0x02,
    THREAD_SUBTYPE_USB_CMD_LAST
};

// usb subType ack
enum {
    THREAD_SUBTYPE_USB_ACK_FIRST = 0x00,
    THREAD_SUBTYPE_USB_ACK_IMAGE = 0x01,
    THREAD_SUBTYPE_USB_ACK_LAST
};

// logger subType cmd
enum {
    THREAD_SUBTYPE_LOGGER_CMD_FIRST = 0x00,
    THREAD_SUBTYPE_LOGGER_CMD_LOG = 0x01, // Ordinary log
    THREAD_SUBTYPE_LOGGER_CMD_LAST
};
typedef struct {
    uint8_t subType;
    CmdCb cb;
} CmdCbPkt;

typedef struct {
    uint8_t type;
    CmdCb cb;
} AckCbPkt;

// protocol for communication between multi-threads
#ifdef __WIN32
#pragma pack(push, 1)
typedef struct {
    uint16_t head;
    uint8_t type;
    uint8_t subType;
    uint16_t seq;
    uint16_t dataLen;
    uint8_t data[THREAD_PACKET_DATA_LEN];
    uint16_t crc;
} ThreadDataPkt;
#pragma pack(pop)
#else
typedef struct __attribute__((__packed__)){
    uint16_t head;
    uint8_t type;
    uint8_t subType;
    uint16_t seq;
    uint16_t dataLen;
    uint8_t data[THREAD_PACKET_DATA_LEN];
    uint16_t crc;
} ThreadDataPkt;
#endif

class QueueManager {
public:
    QueueManager();
    void EnqueueData(const QByteArray &data);
    QByteArray DequeueData();
    bool IsQueueEmpty() const;

private:
    QQueue<QByteArray> queue;
    mutable QMutex mutex;
    QWaitCondition condition; // add QWaitCondition
};

class ThreadManager : public QObject {
    Q_OBJECT
public:
    ThreadManager(int id, ThreadCb cb);
    int GetId() { return id; }
    ThreadCb GetThreadCb() { return taskFunc; }
    void SetThread(QThread *t) { thread = t; }
    QThread *GetThread() { return thread; }

    // Delegate the method of QueueManager
    void EnqueueData(const QByteArray &data) {
        queueManager.EnqueueData(data);
        QCoreApplication::postEvent(this, new QEvent(QEvent::User));
    }
    QByteArray DequeueData() { return queueManager.DequeueData(); }
    bool IsQueueEmpty() const { return queueManager.IsQueueEmpty(); }

public slots:
    void run();

protected:
    void customEvent(QEvent *event) override;

signals:
    void dataProcessed(); // add sign

private:
    int id;
    ThreadCb taskFunc;
    QThread *thread;
    QueueManager queueManager;
};

template <typename T>
struct Node {
    int id;
    T data; // in threadList, data means task
    Node *next;

    Node(int id = 0, T data = nullptr, Node* next = nullptr) : id(id), data(data), next(next) {}
};

template <typename T>
class LinkedList {
private:
    Node<T>* head;
    int nodeNum;

public:
    LinkedList() : head(nullptr), nodeNum(0) {};
    ~LinkedList();

    // CRUD
    void Append(int id, T data);
    void Remove(int id);
    void Update(int id, T data);
    T Get(int id);

    int GetNodeNum();
};

extern LinkedList<ThreadManager *> *g_threadList;

void RegisterThread(LinkedList<ThreadManager *> &list, ThreadManager &cb);
void UnregisterThread(LinkedList<ThreadManager *> &list, int id);
void CreateMultiThreads(LinkedList<ThreadManager *> &threadList);
void MakeThreadDataPkt(uint8_t type, uint8_t subType, QByteArray &src, QByteArray &dst);

#endif // MULTI_THREADS_H
