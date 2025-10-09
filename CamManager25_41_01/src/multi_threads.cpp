#include <QCoreApplication>
#include <QRunnable>
#include <QDebug>
#include <QThread>
#include <QAbstractEventDispatcher>
#include "multi_threads.h"
#include "common.h"
#include "crc.h"

QueueManager::QueueManager()
{
}

void QueueManager::EnqueueData(const QByteArray &data)
{
    QMutexLocker locker(&mutex);
    queue.enqueue(data);
    condition.wakeAll(); // Correct to condition
}

QByteArray QueueManager::DequeueData()
{
    QMutexLocker locker(&mutex);
    if (queue.isEmpty()) {
        condition.wait(&mutex, 1000); // Correct to condition, delay 1 s
        if (queue.isEmpty()) {
            return QByteArray();
        }
    }
    QByteArray data = queue.dequeue();
    return data;
}

bool QueueManager::IsQueueEmpty() const
{
    QMutexLocker locker(&mutex);
    return queue.isEmpty();
}

template <typename T>
LinkedList<T>::~LinkedList()
{
    Node<T>* current = head;
    while (current != nullptr) {
        Node<T>* temp = current;
        current = current->next;
        delete temp;
    }
}

template <typename T>
void LinkedList<T>::Append(int id, T data)
{
    if (head == nullptr) {
        head = new Node<T>(0, data);
    } else {
        Node<T>* current = head;
        while (current->next != nullptr) {
            current = current->next;
        }
        current->next = new Node<T>(id, data);
    }
    nodeNum++;
}

template <typename T>
void LinkedList<T>::Remove(int id)
{
    if (head == nullptr) {
        return;
    }

    if (head->data->GetId() == id) {
        Node<T>* temp = head;
        head = head->next;
        delete temp;
        nodeNum--;
        return;
    }

    Node<T>* current = head;
    while (current->next != nullptr && current->next->data->GetId() != id) {
        current = current->next;
    }

    if (current->next != nullptr) {
        Node<T>* temp = current->next;
        current->next = temp->next;
        delete temp;
        nodeNum--;
    }
}

template <typename T>
void LinkedList<T>::Update(int id, T data)
{
    Node<T>* current = head;
    while (current != nullptr && current->data->GetId() != id) {
        current = current->next;
    }

    if (current != nullptr) {
        current->data = data;
    }
}

template <typename T>
T LinkedList<T>::Get(int id)
{
    Node<T>* current = head;
    while (current != nullptr && current->data->GetId() != id) {
        current = current->next;
    }

    if (current != nullptr) {
        return current->data;
    }
    return nullptr;
}

template <typename T>
int LinkedList<T>::GetNodeNum()
{
    return nodeNum;
}

void RegisterThread(LinkedList<ThreadManager*>& list, ThreadManager& thread)
{
    list.Append(thread.GetId(), &thread);
}

void UnregisterThread(LinkedList<ThreadManager*>& list, int id)
{
    list.Remove(id);
}

void CreateMultiThreads(LinkedList<ThreadManager*>& threadList)
{
    for (int i = 0; i < threadList.GetNodeNum(); ++i) {
        ThreadManager* threadNode = threadList.Get(i);
        if (threadNode && threadNode->GetThread() == nullptr) { // 只对未move的对象操作
            QThread* thread = new QThread();
            threadNode->SetThread(thread);
            threadNode->moveToThread(thread);
            QObject::connect(thread, &QThread::started, threadNode, &ThreadManager::run, Qt::QueuedConnection);
            QObject::connect(threadNode, &ThreadManager::dataProcessed, thread, &QThread::quit, Qt::DirectConnection);
            thread->start();
        }
    }
}

void MakeThreadDataPkt(uint8_t type, uint8_t subType, QByteArray& src, QByteArray& dst)
{
    ThreadDataPkt pkt;
    memset(&pkt, 0, sizeof(pkt));
    uint16_t crc = 0;

    pkt.head = hton16(THREAD_PACKET_HEAD);
    pkt.type = type;
    pkt.subType = subType;
    pkt.seq = 0;
    uint16_t dataLen = qMin(src.size(), (int)(THREAD_PACKET_DATA_LEN));
    pkt.dataLen = hton16(dataLen);
    memcpy(pkt.data, src.constData(), dataLen);

    QByteArray tmp(reinterpret_cast<const char*>(&pkt), 8 + dataLen);
    crc = calc_crc16((const uint8_t*)tmp.data(), tmp.size());
    crc = hton16(crc);
    pkt.crc = crc;

    dst.clear();
    dst.append(reinterpret_cast<const char*>(&pkt), sizeof(pkt));

    PrintBuf(reinterpret_cast<const char*>(&pkt), 8 + dataLen, "MakeThreadDataPkt:");
    const ThreadDataPkt *pktPtr = reinterpret_cast<const ThreadDataPkt*>(dst.constData());
}

ThreadManager::ThreadManager(int id, ThreadCb cb) : id(id), taskFunc(cb), thread(nullptr), queueManager()
{
}

void ThreadManager::run()
{
    //qDebug() << "ThreadManager::run started, id=" << id;
    while (!QThread::currentThread()->isInterruptionRequested()) {
        if (!queueManager.IsQueueEmpty()) {
            QCoreApplication::postEvent(this, new QEvent(QEvent::User));
        }
        QCoreApplication::processEvents(QEventLoop::AllEvents);
        QThread::msleep(100); // Increase sleep time
    }
}

void ThreadManager::customEvent(QEvent* event)
{
    //qDebug() << "ThreadManager::customEvent triggered, id=" << id;
    Q_UNUSED(event);
    if (!queueManager.IsQueueEmpty()) {
        QByteArray data = queueManager.DequeueData();
        //qDebug() << "Thread" << id << "dequeued data, size:" << data.size();
        if (taskFunc) {
            //qDebug() << "Thread" << id << "calling taskFunc";
            taskFunc(data.data());
        } else {
            //qDebug() << "Thread" << id << "taskFunc is null";
        }
        emit dataProcessed();
    }
    // Remove the output "queue is empty in customEvent"
}

template class LinkedList<ThreadManager*>;
